# STM32 application builder
#
# Author: Marcelo Barros de Almeida <marcelobarrosalmeida@gmail.com>
# License: BSD
# 

include globals.inc

# application to build
APP ?= app

BLDDIR=$(G_BLDDIR)

SUBDIRS = stm32f4  $(APP)

BUILDDIRS=$(SUBDIRS:%=build-%)
CLEANDIRS=$(SUBDIRS:%=clean-%)
CSCOPEDIRS=$(SUBDIRS:%=cscope-%)

all: $(BUILDDIRS)

clean: $(CLEANDIRS)
	-rm -fR $(BLDDIR)/*

clean-cscope:
	-@mkdir $(BLDDIR) 2>/dev/null || true
	-@rm $(BLDDIR)/cscope.*

cscope: clean-cscope $(CSCOPEDIRS)
	@echo -e "\n\nGo to output directory and type cscope\n"

$(SUBDIRS): $(BUILDDIRS)

$(BUILDDIRS):
	@echo -e "Building $(@:build-%=%) ...\n"
	$(MAKE) -C $(@:build-%=%) APP=$(APP)

$(CLEANDIRS):
	@echo -e "Cleaning $(@:build-%=%) ...\n"
	$(MAKE) -C $(@:clean-%=%) clean APP=$(APP)

$(CSCOPEDIRS):
	@echo -e "Generating cscope for $(@:build-%=%) ...\n"
	$(MAKE) -C $(@:cscope-%=%) cscope APP=$(APP)

help:
	@echo ""
	@echo "  make                : compila a aplicação app (nome default da aplicação/diretório)"
	@echo ""
	@echo "  Para gerar outras aplicações que não estejam no diretório app, use:"
	@echo ""
	@echo "  make APP=app_dir_name"
	@echo ""
	@echo "  Recomenda-se executar uma limpeza antes de compilar a sua aplicação pois a biblioteca"
	@echo "  pode sofrer alterações (o hal conf pode ser diferente do último usado)"
	@echo ""
	@echo "  Outros targets"
	@echo ""
	@echo "  make clean          : apaga todos os temporários"
	@echo "  make clean-app1     : apaga todos os temporários da aplicação app1"
	@echo "  make build-app1     : compila a aplicação app1"
	@echo "  make cscope         : gera os arquivos para cscope"
	@echo ""
	@echo "Opções:"
	@echo ""
	@echo "1) Debug x Release"
	@echo ""
	@echo "   O padrão é gerar com debug (-g)."
	@echo "   Para uma versão release, acrescente RELEASE=ON na linha de comando"
	@echo ""
	@echo "   make RELEASE=ON"
	@echo ""

.PHONY: subdirs
.PHONY: $(SUBDIRS) $(BUILDDIRS) $(CLEANDIRS) $(CSCOPEDIRS)
.PHONY: all clean cscope
.PHONY: clean-cscope
