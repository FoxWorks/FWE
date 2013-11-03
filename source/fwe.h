////////////////////////////////////////////////////////////////////////////////
/// @file
////////////////////////////////////////////////////////////////////////////////
/// Copyright (C) 2012-2013, Black Phoenix
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU General Public License as published by
/// the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
///
/// You should have received a copy of the GNU General Public License
/// along with this program.  If not, see http://www.gnu.org/licenses/.
////////////////////////////////////////////////////////////////////////////////
#ifndef FWE_H
#define FWE_H
#ifdef __cplusplus
extern "C" {
#endif

/// Run editor as a standalone application
#define FOXWORKS_EDITOR_STANDALONE		1
/// Execute editor in a different thread
#define FOXWORKS_EDITOR_BLOCKING		2

void fw_editor_initialize(int flags, int argc, char *argv[]);
void fw_editor_deinitialize();
void fw_editor_frame();
//void fw_editor_render();

extern int fw_editor_flags;

#ifdef __cplusplus
}
#endif
#endif