# Copyright (C) 2015  Vianney le Clément de Saint-Marcq <vleclement@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 3 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

.PHONY: all
all: bootimgtool

CFLAGS=-Wall -std=c99

ifdef DEBUG
CFLAGS+=-g -DDEBUG
endif

bootimgtool: bootimgtool.o io.o variant_standard.o
bootimgtool.o variant_standard.o: bootimgtool.h bootimg.h io.h
io.o: io.h

.PHONY: clean
clean:
	rm -f bootimgtool *.o
