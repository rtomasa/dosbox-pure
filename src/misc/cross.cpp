/*
 *  Copyright (C) 2002-2021  The DOSBox Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


#include "dosbox.h"
#include "cross.h"
#include "support.h"
#include <string>
#include <limits.h>
#include <stdlib.h>

#if defined(C_DBP_ENABLE_CONFIG_PROGRAM) || defined(C_DBP_ENABLE_CAPTURE) || defined(C_OPENGL)
#ifdef WIN32
#ifndef _WIN32_IE
#define _WIN32_IE 0x0400
#endif
#include <shlobj.h>
#endif
#endif

#ifdef C_DBP_NATIVE_HOMEDIR
#if defined HAVE_SYS_TYPES_H && defined HAVE_PWD_H
#include <sys/types.h>
#include <pwd.h>
#endif
#endif

#if defined(C_DBP_ENABLE_CONFIG_PROGRAM) || defined(C_DBP_ENABLE_CAPTURE) || defined(C_OPENGL)
#ifdef WIN32
static void W32_ConfDir(std::string& in,bool create) {
	int c = create?1:0;
	char result[MAX_PATH] = { 0 };
	BOOL r = SHGetSpecialFolderPath(NULL,result,CSIDL_LOCAL_APPDATA,c);
	if(!r || result[0] == 0) r = SHGetSpecialFolderPath(NULL,result,CSIDL_APPDATA,c);
	if(!r || result[0] == 0) {
		char const * windir = getenv("windir");
		if(!windir) windir = "c:\\windows";
		safe_strncpy(result,windir,MAX_PATH);
		char const* appdata = "\\Application Data";
		size_t len = strlen(result);
		if(len + strlen(appdata) < MAX_PATH) strcat(result,appdata);
		if(create) mkdir(result);
	}
	in = result;
}
#endif

void Cross::GetPlatformConfigDir(std::string& in) {
#ifdef WIN32
	W32_ConfDir(in,false);
	in += "\\DOSBox";
#elif defined(MACOSX)
	in = "~/Library/Preferences";
	ResolveHomedir(in);
#else
	in = "~/.dosbox";
	ResolveHomedir(in);
#endif
	in += CROSS_FILESPLIT;
}

void Cross::GetPlatformConfigName(std::string& in) {
#ifdef WIN32
#define DEFAULT_CONFIG_FILE "dosbox-" VERSION ".conf"
#elif defined(MACOSX)
#define DEFAULT_CONFIG_FILE "DOSBox " VERSION " Preferences"
#else /*linux freebsd*/
#define DEFAULT_CONFIG_FILE "dosbox-" VERSION ".conf"
#endif
	in = DEFAULT_CONFIG_FILE;
}

void Cross::CreatePlatformConfigDir(std::string& in) {
#ifdef WIN32
	W32_ConfDir(in,true);
	in += "\\DOSBox";
	mkdir(in.c_str());
#elif defined(MACOSX)
	in = "~/Library/Preferences";
	ResolveHomedir(in);
	//Don't create it. Assume it exists
#else
	in = "~/.dosbox";
	ResolveHomedir(in);
	mkdir(in.c_str(),0700);
#endif
	in += CROSS_FILESPLIT;
}

void Cross::ResolveHomedir(std::string & temp_line) {
#ifdef C_DBP_NATIVE_HOMEDIR
	if(!temp_line.size() || temp_line[0] != '~') return; //No ~

	if(temp_line.size() == 1 || temp_line[1] == CROSS_FILESPLIT) { //The ~ and ~/ variant
		char * home = getenv("HOME");
		if(home) temp_line.replace(0,1,std::string(home));
#if defined HAVE_SYS_TYPES_H && defined HAVE_PWD_H && !defined HAVE_LIBNX && !defined PSP
	} else { // The ~username variant
		std::string::size_type namelen = temp_line.find(CROSS_FILESPLIT);
		if(namelen == std::string::npos) namelen = temp_line.size();
		std::string username = temp_line.substr(1,namelen - 1);
		struct passwd* pass = getpwnam(username.c_str());
		if(pass) temp_line.replace(0,namelen,pass->pw_dir); //namelen -1 +1(for the ~)
#endif // USERNAME lookup code
	}
#endif
}

void Cross::CreateDir(std::string const& in) {
#ifdef WIN32
	mkdir(in.c_str());
#else
	mkdir(in.c_str(),0700);
#endif
}

bool Cross::IsPathAbsolute(std::string const& in) {
	// Absolute paths
#if defined (WIN32) || defined(OS2)
	// drive letter
	if (in.size() > 2 && in[1] == ':' ) return true;
	// UNC path
	else if (in.size() > 2 && in[0]=='\\' && in[1]=='\\') return true;
#else
	if (in.size() > 1 && in[0] == '/' ) return true;
#endif
	return false;
}
#else //defined(C_DBP_ENABLE_CONFIG_PROGRAM) || defined(C_DBP_ENABLE_CAPTURE) || defined(C_OPENGL)
std::string& Cross::MakePathAbsolute(std::string& str)
{
	size_t strsz = str.size();
	#ifdef WIN32
	if (strsz > 2 && (str[1] == ':' || (str[0]=='\\' && str[1]=='\\'))) return str;
	wchar_t buf[512]; UINT cp; int len;
	GetCurrentDirectoryW(512, buf);
	if ((len = WideCharToMultiByte(CP_UTF8, 0, buf, -1, NULL, 0, NULL, NULL)) > 0) cp = CP_UTF8;
	else if ((len = WideCharToMultiByte((cp = CP_ACP), 0, buf, -1, NULL, 0, NULL, NULL)) <= 0) return str; // fall back to ANSI codepage
	str.insert(0, len, ' '); // len includes \0 terminator
	WideCharToMultiByte(cp, 0, buf, -1, &str[0], len, NULL, NULL);
	if (strsz) str[len - 1] = '\\'; // swap \0 to path separator
	else str.resize(len - 1); // truncate without \0 terminator
	#else
	if (strsz > 1 && str[0] == '/' ) return str;
	char buf[512];
	if (!getcwd(buf, 510)) return str;
	if (strsz) strcat(buf, "/");
	str.insert(0, buf);
	#endif
	return str;
}
std::string& Cross::NormalizePath(std::string& str) // strip ., .., repeated / and trailing / (unless root), standardize path separator
{
	if (!*str.c_str()) return str; // c_str guarantees \0 terminator afterwards
	for (char *path = &str[0], *src = path, *dst = path, *root = src + (*src == '/' || *src == '\\'), c;;)
	{
		if ((c = *(src++)) != '.')
		{
			if (c == '\0') { str.resize(dst - (dst > root && dst[-1] == CROSS_FILESPLIT) - path); return str; }
			else if (c != '/' && c != '\\') *(dst++) = c;
			#ifdef WIN32
			else if (src - path == 1 && c == '\\' && *src == '\\') { src++; dst += 2; } // network path
			#endif
			else if (dst != root && !(dst > path && dst[-1] == CROSS_FILESPLIT)) { *(dst++) = CROSS_FILESPLIT; }
		}
		else if ((dst == path || dst[-1] == CROSS_FILESPLIT) && (src[0] == '/' || src[0] == '\\' || src[0] == '\0')) src += 0 + (src[0] != '\0');
		else if (dst == path || dst[-1] != CROSS_FILESPLIT || src[0] != '.' || (src[1] != '/' && src[1] != '\\' && src[1] != '\0') || (dst >= path + 3 && dst[-2] == '.' && dst[-3] == '.' && (dst == path + 3 || dst[-4] == CROSS_FILESPLIT))) *(dst++) = '.';
		else { src += 1 + (src[1] != '\0'); if (dst > root) { for (dst--; dst[-1] != CROSS_FILESPLIT;) { if (--dst == root) break; } } }
	}
}
#endif

#if defined (WIN32)

dir_information* open_directory(const char* dirname) {
	if (dirname == NULL) return NULL;

	size_t len = strlen(dirname);
	if (len == 0) return NULL;

	static dir_information dir;

	safe_strncpy(dir.base_path,dirname,MAX_PATH);

	if (dirname[len-1] == '\\') strcat(dir.base_path,"*.*");
	else                        strcat(dir.base_path,"\\*.*");

	dir.handle = INVALID_HANDLE_VALUE;

	return (access(dirname,0) ? NULL : &dir);
}

bool read_directory_first(dir_information* dirp, char* entry_name, bool& is_directory) {
	if (!dirp) return false;
	dirp->handle = FindFirstFile(dirp->base_path, &dirp->search_data);
	if (INVALID_HANDLE_VALUE == dirp->handle) {
		return false;
	}

	safe_strncpy(entry_name,dirp->search_data.cFileName,(MAX_PATH<CROSS_LEN)?MAX_PATH:CROSS_LEN);

	if (dirp->search_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) is_directory = true;
	else is_directory = false;

	return true;
}

bool read_directory_next(dir_information* dirp, char* entry_name, bool& is_directory) {
	if (!dirp) return false;
	int result = FindNextFile(dirp->handle, &dirp->search_data);
	if (result==0) return false;

	safe_strncpy(entry_name,dirp->search_data.cFileName,(MAX_PATH<CROSS_LEN)?MAX_PATH:CROSS_LEN);

	if (dirp->search_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) is_directory = true;
	else is_directory = false;

	return true;
}

void close_directory(dir_information* dirp) {
	if (dirp && dirp->handle != INVALID_HANDLE_VALUE) {
		FindClose(dirp->handle);
		dirp->handle = INVALID_HANDLE_VALUE;
	}
}

#else

dir_information* open_directory(const char* dirname) {
	static dir_information dir;
	dir.dir=opendir(dirname);
	safe_strncpy(dir.base_path,dirname,CROSS_LEN);
	return dir.dir?&dir:NULL;
}

bool read_directory_first(dir_information* dirp, char* entry_name, bool& is_directory) {
	if (!dirp) return false;
	return read_directory_next(dirp,entry_name,is_directory);
}

bool read_directory_next(dir_information* dirp, char* entry_name, bool& is_directory) {
	if (!dirp) return false;
	struct dirent* dentry = readdir(dirp->dir);
	if (dentry==NULL) {
		return false;
	}

//	safe_strncpy(entry_name,dentry->d_name,(FILENAME_MAX<MAX_PATH)?FILENAME_MAX:MAX_PATH);	// [include stdio.h], maybe pathconf()
	safe_strncpy(entry_name,dentry->d_name,CROSS_LEN);

#ifdef DIRENT_HAS_D_TYPE
	if(dentry->d_type == DT_DIR) {
		is_directory = true;
		return true;
	} else if(dentry->d_type == DT_REG) {
		is_directory = false;
		return true;
	}
#endif

	//Maybe only for DT_UNKNOWN if DIRENT_HAD_D_TYPE..
	static char buffer[2 * CROSS_LEN + 1] = { 0 };
	static char split[2] = { CROSS_FILESPLIT , 0 };
	buffer[0] = 0;
	strcpy(buffer,dirp->base_path);
	size_t buflen = strlen(buffer);
	if (buflen && buffer[buflen - 1] != CROSS_FILESPLIT ) strcat(buffer, split);
	strcat(buffer,entry_name);
	struct stat status;

	if (stat(buffer,&status) == 0) is_directory = (S_ISDIR(status.st_mode)>0);
	else is_directory = false;

	return true;
}

void close_directory(dir_information* dirp) {
	if (dirp) closedir(dirp->dir);
}

#endif

#ifdef C_DBP_USE_SDL
FILE *fopen_wrap(const char *path, const char *mode) {
#if defined(WIN32) || defined(OS2)
	;
#elif defined (MACOSX)
	;
#else  
#if defined (HAVE_REALPATH)
	char work[CROSS_LEN] = {0};
	strncpy(work,path,CROSS_LEN-1);
	char* last = strrchr(work,'/');
	
	if (last) {
		if (last != work) {
			*last = 0;
			//If this compare fails, then we are dealing with files in / 
			//Which is outside the scope, but test anyway. 
			//However as realpath only works for exising files. The testing is 
			//in that case not done against new files.
		}
		char* check = realpath(work,NULL);
		if (check) {
			if ( ( strlen(check) == 5 && strcmp(check,"/proc") == 0) || strncmp(check,"/proc/",6) == 0) {
//				LOG_MSG("lst hit %s blocking!",path);
				free(check);
				return NULL;
			}
			free(check);
		}
	}

#if 0
//Lightweight version, but then existing files can still be read, which is not ideal	
	if (strpbrk(mode,"aw+") != NULL) {
		LOG_MSG("pbrk ok");
		char* check = realpath(path,NULL);
		//Will be null if file doesn't exist.... ENOENT
		//TODO What about unlink /proc/self/mem and then create it ?
		//Should be safe for what we want..
		if (check) {
			if (strncmp(check,"/proc/",6) == 0) {
				free(check);
				return NULL;
			}
			free(check);
		}
	}
*/
#endif //0 

#endif //HAVE_REALPATH
#endif

	return fopen(path,mode);
}
#endif
