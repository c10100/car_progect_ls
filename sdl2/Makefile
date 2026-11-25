#
# Makefile
#
CC ?= gcc
LVGL_DIR_NAME ?= lvgl
LVGL_DIR ?= ${shell pwd}
CFLAGS ?= -O3 -g0 -I$(LVGL_DIR)/ -I$(LVGL_DIR)/lvgl -I$(LVGL_DIR)/mycode/inc -I$(LVGL_DIR)/mycode/include -I$(LVGL_DIR)/mycode/third_party/cjson -I$(LVGL_DIR)/sq_UI -DCARPAD_PC_SIM -Wall -Wshadow -Wundef -Wmissing-prototypes -Wno-discarded-qualifiers -Wall -Wextra -Wno-unused-function -Wno-error=strict-prototypes -Wpointer-arith -fno-strict-aliasing -Wno-error=cpp -Wuninitialized -Wmaybe-uninitialized -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits -Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wno-cast-qual -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wformat-security -Wno-ignored-qualifiers -Wno-error=pedantic -Wno-sign-compare -Wno-error=missing-prototypes -Wdouble-promotion -Wclobbered -Wdeprecated -Wempty-body -Wtype-limits -Wshift-negative-value -Wstack-usage=2048 -Wno-unused-value -Wno-unused-parameter -Wno-missing-field-initializers -Wuninitialized -Wmaybe-uninitialized -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits -Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wpointer-arith -Wno-cast-qual -Wmissing-prototypes -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wno-discarded-qualifiers -Wformat-security -Wno-ignored-qualifiers -Wno-sign-compare
LDFLAGS ?= -lm -lpthread
BIN = demo

#SDL2 flags (Linux/WSL simulator)
SDL_CFLAGS = $(shell sdl2-config --cflags)
SDL_LIBS = $(shell sdl2-config --libs)


#Collect the files to compile
MAINSRC = ./main.c

include $(LVGL_DIR)/lvgl/lvgl.mk
include $(LVGL_DIR)/lv_drivers/lv_drivers.mk

CSRCS +=$(LVGL_DIR)/mouse_cursor_icon.c
# mycode 业务代码: 递归收集 src 下全部 .c, 排除 src/main.c (入口用本目录 main.c)
CSRCS +=$(shell find $(LVGL_DIR)/mycode/src -name '*.c' ! -name 'main.c')
CSRCS +=$(LVGL_DIR)/mycode/third_party/cjson/cJSON.c
# sq_UI 课程示例 UI 与 mycode 的 CARPAD OS 都定义 ui_init, 二选一; 想看示例时放开:
#CSRCS +=$(wildcard $(LVGL_DIR)/sq_UI/*.c)
#CSRCS +=$(wildcard $(LVGL_DIR)/sq_UI/screens/*.c)
#CSRCS +=$(wildcard $(LVGL_DIR)/sq_UI/images/*.c)
#CSRCS +=$(wildcard $(LVGL_DIR)/sq_UI/fonts/*.c)

#=============================================

OBJEXT ?= .o

AOBJS = $(ASRCS:.S=$(OBJEXT))
COBJS = $(CSRCS:.c=$(OBJEXT))

MAINOBJ = $(MAINSRC:.c=$(OBJEXT))

SRCS = $(ASRCS) $(CSRCS) $(MAINSRC)
OBJS = $(AOBJS) $(COBJS)

## MAINOBJ -> OBJFILES

all: default
#===============
ifeq ($(findstring n,$(MAKEFLAGS)),n)
DRY_RUN_FORCE := .dry-run-force
endif
.PHONY: .dry-run-force
.dry-run-force: ;
#=============
%.o: %.c $(DRY_RUN_FORCE)
#%.o: %.c
	@$(CC)  $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@
	@echo "CC $<"

default: $(AOBJS) $(COBJS) $(MAINOBJ)
	$(CC) -o $(BIN) $(MAINOBJ) $(AOBJS) $(COBJS) $(LDFLAGS) $(SDL_LIBS)

clean: 
	rm -f $(BIN) $(AOBJS) $(COBJS) $(MAINOBJ)

