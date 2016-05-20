
import cmd as cmdline
from threading import *
from Queue import Queue
from serial import *
import time
import logging 
import os
import shelve
import sys

from constants_485 import *
from cmd_485 import *
from serial_485 import *
from serial import *
if EMULATION:
    from serial_pipe import *

logging.basicConfig(filename='master.log',
                    level=logging.DEBUG,
                    format='%(asctime)s %(message)s',
                    datefmt='%I:%M:%S')

cfg_file_path = os.path.dirname(sys.argv[0])
cfg_file_path = os.path.abspath(cfg_file_path)
cfg_file_name = os.path.join(cfg_file_path,'config.db')

try:
    APP_DATA = shelve.open(cfg_file_name, writeback=True)
except Exception, e:
    logging.debug(str(e))
    if os.path.exists(cfg_file_name):
        os.remove(cfg_file_name)
    APP_DATA = shelve.open(cfg_file_name, flag='n', writeback=True)

class MasterCmd(cmdline.Cmd):
    
    MASTER_ADDR = 0x00

    def __init__(self):
        cmdline.Cmd.__init__(self)

        self.prompt = '>> '
        self.stop = Event()
        self.txq = Queue()
        self.rxq = Queue()

        try:
            if EMULATION:
                self.serial = SerialPipe(True)
            else:
                self.serial = Serial(SERIAL_MASTER,baudrate=SERIAL_BAUD,timeout=SERIAL_TIMEOUT)
        except Exception, e:
            logging.debug(str(e))
            sys.exit(1)

        self.ser485 = Serial485(self.serial,self.rxq, self.txq, self.stop, True)
        self.ser485.start()

        self.recv = Thread(target=self.recv_data)
        self.recv.start()

    def __conv_hex_int(self,s):
        if s.find('x') >= 0 or s.find('X') >= 0:
            n = int(s,16)
        else:
            n = int(s)
            
        return n
             
    def recv_data(self):
        while not self.stop.is_set():
            try:
                cmd = self.rxq.get(True, 1)
            except Exception, e:
                #logging.info(str(e))
                time.sleep(0.5)
                continue

            self.save_data(cmd)
            
            print cmd.beauty()[:-1]  
            self.lastcmd = ''
        
    def save_data(self, cmd):
        dev = str(cmd.src)
        c = cmd.cmd
        
        if dev not in APP_DATA:
            APP_DATA[dev] = {}
            
        if c == SENS_IDENT_CMD:
            APP_DATA[dev]['ident'] = {}
            APP_DATA[dev]['desc'] = {}
        
        if c == SENS_ITF_VER_CMD:
            APP_DATA[dev]['version'] = cmd.version
        elif c == SENS_IDENT_CMD:
            for k,v in cmd.payload.iteritems():
                APP_DATA[dev]['ident'][k] = v
        elif c >= SENS_POINT_DESC_BASE_CMD and c < (SENS_POINT_DESC_BASE_CMD + SENS_MAX_POINTS):
            p = c - SENS_POINT_DESC_BASE_CMD        
            APP_DATA[dev]['desc'][p] = {}
            for k,v in cmd.payload.iteritems():
                APP_DATA[dev]['desc'][p][k] = v

        APP_DATA.sync()
    
    def do_devices(self,buf):
        'List known devices.\nSyntax: devices'
        line = '-'*(3+1+8+1+8+1+8+1+3+1+2+1)
        print line
        print '%-3s %-8s %-8s %-8s %-3s %-s' % ('DEV','MANUF','MODEL','ID','REV','PTS')
        print line
        keys = APP_DATA.keys()
        keys.sort()
        for dev in keys:
            if APP_DATA[dev].has_key('ident'):
                manuf  = APP_DATA[dev]['ident']['manuf']
                model  = APP_DATA[dev]['ident']['model']
                id     = APP_DATA[dev]['ident']['id']
                rev    = APP_DATA[dev]['ident']['rev']
                points = APP_DATA[dev]['ident']['points']
                print '%02X  %-8s %-8s %08X %02X  %02X ' % (int(dev),manuf,model,id,rev,points)                
            else:
                print '%02X  %-8s %-8s %08X %02X  %02X ' % (int(dev),'-'*8,'-'*8,0,0,0)
        print line

    def do_points(self,buf):
        'List known points for specific device.\nSyntax: points dev'
        try:
            dev = self.__conv_hex_int(buf)
        except:
            print "Wrong arguments"
            return  
       
        dev = str(dev)
        if dev in APP_DATA.keys():
            if APP_DATA[dev].has_key('desc'):
                line = '-'*(3+1+8+1+4+1+4+1+6)
                print line
                print '%-3s %-8s %-4s %-4s %-6s' % ('NUM','NAME','TYPE','UNIT','RIGHTS')
                print line
                keys = APP_DATA[dev]['desc'].keys()
                keys.sort()
                for p in keys:
                    name  = APP_DATA[dev]['desc'][p]['name']
                    type  = APP_DATA[dev]['desc'][p]['type']
                    unit  = APP_DATA[dev]['desc'][p]['unit']
                    rights = APP_DATA[dev]['desc'][p]['rights']
                    rights = POINT_RIGHTS[rights]
                    print '%02X  %-8s %02X   %02X   %-2s' % (p,name,type,unit,rights)                
                print line             
            else:
                print 'Description not available'
        else:
            print 'Device not available'                    
                    
    def do_quit(self,buf):
        'Quit program.\nSyntax: quit'
        self.stop.set()
        time.sleep(1)
        if not EMULATION:
            self.serial.close()
        sys.exit(0)

    def do_version(self,buf):
        'Read device interface version.\nSyntax: version dev'
        try:
            dev = self.__conv_hex_int(buf)
        except:
            print "Wrong arguments"
            return        
        c = Cmd(cmd=SENS_ITF_VER_CMD,dst=dev,src=MasterCmd.MASTER_ADDR)
        #logging.debug(c)
        self.txq.put(c)

    def do_ident(self,buf):
        'Read device identification.\nSyntax: ident dev'
        try:
            dev = self.__conv_hex_int(buf)
        except:
            print "Wrong arguments"
            return        
        c = Cmd(cmd=SENS_IDENT_CMD,dst=dev,src=MasterCmd.MASTER_ADDR)
        #logging.debug(c)
        self.txq.put(c)

    def do_clear(self,buf):
        'Clear database.\nSyntax: clear'
        for k in APP_DATA.keys():
            del APP_DATA[k]
        APP_DATA.sync()
        print "Database erased."

    def do_desc(self,buf):
        'Read point description.\nSyntax: desc dev point'
        try:
            (dev,point) = buf.split()
            dev = self.__conv_hex_int(dev)
            point = self.__conv_hex_int(point)
        except:
            print "Wrong arguments"
            return        
        c = Cmd(cmd=SENS_POINT_DESC_BASE_CMD+point,dst=dev,src=MasterCmd.MASTER_ADDR)
        #logging.debug(c)
        self.txq.put(c)
        
    def __conv_value(self,value,unit):
        if unit <= POINT_TYPE_INT64:
            value = self.__conv_hex_int(value)
        else:
            value = float(value)
        
        return value
        
    def do_write(self,buf):
        'Write point value.\nSyntax: write dev point value'
        try:
            (dev,point,value) = buf.split()
            dev = self.__conv_hex_int(dev)
            point = self.__conv_hex_int(point)
        except:
            print "Wrong arguments"
            return        
            
        dev = str(dev)
        
        if dev not in APP_DATA.keys():
            print 'Device missing'
            return
            
        if not APP_DATA[dev].has_key('desc'):
            print 'Description missing'
            return
            
        if point not in APP_DATA[dev]['desc']:
            print 'Point description missing'
            return
        
        unit  = APP_DATA[dev]['desc'][point]['unit']
        type  = APP_DATA[dev]['desc'][point]['type']
        
        try:
            value = self.__conv_value(value,unit)
        except:
            print 'Invalid value for unit %d' % unit
            return
            
        c = Cmd(cmd=SENS_POINT_WRITE_BASE_CMD+point,dst=int(dev),src=MasterCmd.MASTER_ADDR,value=value,type=type)
        #logging.debug(c)
        self.txq.put(c)        
        
        
    def do_read(self,buf):
        'Read point value.\nSyntax: read dev point'
        try:
            (dev,point) = buf.split()
            dev = self.__conv_hex_int(dev)
            point = self.__conv_hex_int(point)
        except:
            print "Wrong arguments"
            return  
                    
        c = Cmd(cmd=SENS_POINT_READ_BASE_CMD+point,dst=int(dev),src=MasterCmd.MASTER_ADDR)
        #logging.debug(c)
        self.txq.put(c) 
                            
loop = MasterCmd()
loop.cmdloop()

