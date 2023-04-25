##
# MOD
# this file is used only for building the pc version
# add liballeg and brotli and other minor fixes
################################################################
#
# $Id:$
#
# $Log:$
#
CC=  gcc  # gcc or g++

CFLAGS=-m32 -g -Wall -DNORMALUNIX -DLINUX -DUSE_PC_PORT# -DUSEASM 
LDFLAGS=-L/usr/X11R6/lib
#LIBS=-lXext -lX11 -lnsl -lm
LIBS=-lm -lallegro -lallegro_image -lallegro_main -lallegro_ttf -lallegro_primitives -lallegro_audio -lallegro_font

# subdirectory for objects
O=linux

# not too sophisticated dependency
OBJS=				\
		$(O)/doomdef.o		\
		$(O)/doomstat.o		\
		$(O)/dstrings.o		\
		$(O)/i_system.o		\
		$(O)/i_sound.o		\
		$(O)/i_video.o		\
		$(O)/i_net.o			\
		$(O)/tables.o			\
		$(O)/f_finale.o		\
		$(O)/f_wipe.o 		\
		$(O)/d_main.o			\
		$(O)/d_net.o			\
		$(O)/d_items.o		\
		$(O)/g_game.o			\
		$(O)/m_menu.o			\
		$(O)/m_misc.o			\
		$(O)/m_argv.o  		\
		$(O)/m_bbox.o			\
		$(O)/m_fixed.o		\
		$(O)/m_swap.o			\
		$(O)/m_cheat.o		\
		$(O)/m_random.o		\
		$(O)/am_map.o			\
		$(O)/p_ceilng.o		\
		$(O)/p_doors.o		\
		$(O)/p_enemy.o		\
		$(O)/p_floor.o		\
		$(O)/p_inter.o		\
		$(O)/p_lights.o		\
		$(O)/p_map.o			\
		$(O)/p_maputl.o		\
		$(O)/p_plats.o		\
		$(O)/p_pspr.o			\
		$(O)/p_setup.o		\
		$(O)/p_sight.o		\
		$(O)/p_spec.o			\
		$(O)/p_switch.o		\
		$(O)/p_mobj.o			\
		$(O)/p_telept.o		\
		$(O)/p_tick.o			\
		$(O)/p_saveg.o		\
		$(O)/p_user.o			\
		$(O)/r_bsp.o			\
		$(O)/r_data.o			\
		$(O)/r_draw.o			\
		$(O)/r_main.o			\
		$(O)/r_plane.o		\
		$(O)/r_segs.o			\
		$(O)/r_sky.o			\
		$(O)/r_things.o		\
		$(O)/w_wad.o			\
		$(O)/wi_stuff.o		\
		$(O)/v_video.o		\
		$(O)/st_lib.o			\
		$(O)/st_stuff.o		\
		$(O)/hu_stuff.o		\
		$(O)/hu_lib.o			\
		$(O)/s_sound.o		\
		$(O)/z_zone.o			\
		$(O)/info.o				\
		$(O)/sounds.o          \
		$(O)/brotli/common/constants.o          \
		$(O)/brotli/common/context.o            \
		$(O)/brotli/common/dictionary.o         \
		$(O)/brotli/common/platform.o           \
		$(O)/brotli/common/shared_dictionary.o  \
		$(O)/brotli/common/transform.o          \
		$(O)/brotli/dec/bit_reader.o            \
		$(O)/brotli/dec/decode.o                \
		$(O)/brotli/dec/huffman.o               \
		$(O)/brotli/dec/state.o                 \

all:	dirs $(O)/linuxxdoom

dirs:
	mkdir -p $(O)/brotli/common
	mkdir -p $(O)/brotli/dec

clean:
	rm -f *.o *~ *.flc
	rm -f linux/*.o
	rm -rf linux/brotli
	rm -f linux/linuxxdoom.exe

$(O)/linuxxdoom:	$(OBJS) $(O)/i_main.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(O)/i_main.o \
	-o $(O)/linuxxdoom $(LIBS)

$(O)/%.o:	%.c
	$(CC) $(CFLAGS) -c $< -o $@ -Ibrotli/include

#############################################################
#
#############################################################