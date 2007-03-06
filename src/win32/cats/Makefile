#
# Makefile for win32 bacula executables
# Using MinGW cross-compiler on GNU/Linux
#
#  Written by Robert Nelson, June 2006
#

include ../Makefile.inc

INCLUDES = \
	$(INCLUDE_PTHREADS) \
	$(INCLUDE_BACULA) \
	$(INCLUDE_ZLIB) \
	$(INCLUDE_OPENSSL) \
	$(INCLUDE_MYSQL)

DEFINES = \
	$(HAVES)

vpath %.c ../../cats
vpath %.cpp ../../cats

######################################################################

# Files files in src/lib

SQL_OBJS = \
	sql.o \
	sql_cmds.o \
	sql_create.o \
	sql_delete.o \
	sql_find.o \
	sql_get.o \
	sql_list.o \
	sql_update.o \

LIBS_DLL = \
	$(LIBS_BACULA)

######################################################################

# Targets

.PHONY: all clean

all: $(LIBDIR)/libcats.a $(BINDIR)/cats_mysql.dll $(BINDIR)/cats_postgresql.dll $(BINDIR)/cats_sqlite3.dll

clean:
	@echo "Cleaning `pwd`"
	$(call clean_obj,$(addprefix $(OBJDIR)/cats_mysql/,mysql.o $(SQL_OBJS)))
	$(call clean_obj,$(addprefix $(OBJDIR)/cats_postgresql/,postgresql.o $(SQL_OBJS)))
	$(call clean_obj,$(addprefix $(OBJDIR)/cats_sqlite3/,sqlite.o $(SQL_OBJS)))
	$(call clean_exe,$(BINDIR)/cats_mysql.dll)
	$(call clean_exe,$(BINDIR)/cats_postgresql.dll)
	$(call clean_exe,$(BINDIR)/cats_sqlite3.dll)
	$(ECHO_CMD)rm -f $(OBJDIR)/libcats.exp $(LIBDIR)/libcats.a

$(LIBDIR)/libcats.a $(OBJDIR)/libcats.exp: bacula_cats.def
	$(DLLTOOL) --dllname bacula_cats.dll --no-export-all-symbols --input-def bacula_cats.def --output-exp $(OBJDIR)/libcats.exp --output-lib $(LIBDIR)/libcats.a $^

$(BINDIR)/cats_mysql.dll: $(addprefix $(OBJDIR)/cats_mysql/,mysql.o $(SQL_OBJS)) $(OBJDIR)/libcats.exp
	@echo "Linking $@"
	$(call checkdir,$@)
	$(ECHO_CMD)$(CXX) $(LDFLAGS) -mdll -mwindows $^ $(LIBS_MYSQL) $(LIBS_DLL) -o $@

$(BINDIR)/cats_postgresql.dll: $(addprefix $(OBJDIR)/cats_postgresql/,postgresql.o $(SQL_OBJS)) $(OBJDIR)/libcats.exp
	@echo "Linking $@"
	$(call checkdir,$@)
	$(ECHO_CMD)$(CXX) $(LDFLAGS) -mdll -mwindows $^ $(LIBS_POSTGRESQL) $(LIBS_DLL) -o $@

$(BINDIR)/cats_sqlite3.dll: $(addprefix $(OBJDIR)/cats_sqlite3/,sqlite.o $(SQL_OBJS)) $(OBJDIR)/libcats.exp
	@echo "Linking $@"
	$(call checkdir,$@)
	$(ECHO_CMD)$(CXX) $(LDFLAGS) -mdll -mwindows $^ $(LIBS_SQLITE) $(LIBS_DLL) -o $@

#
# Rules for generating from ../cats
#

include ../Makefile.rules

define Link_Dll
$(OBJDIR)/$(1)/%.o: %.c
	@echo "Compiling $$<"
	$$(call checkdir,$$@)
	$(ECHO_CMD)$(CXX) -DBUILDING_CATS -DUSING_DLL -DHAVE_$(2) $(CFLAGS) -c $$< -o $$@

$(OBJDIR)/$(1)/%.o: %.cpp
	@echo "Compiling $$<"
	$$(call checkdir,$$@)
	$(ECHO_CMD)$(CXX) -DBUILDING_CATS -DUSING_DLL -DHAVE_$(2) $(CFLAGS) -c $$< -o $$@

endef

$(eval $(call Link_Dll,cats_mysql,MYSQL))

$(eval $(call Link_Dll,cats_postgresql,POSTGRESQL))

$(eval $(call Link_Dll,cats_sqlite3,SQLITE3))

