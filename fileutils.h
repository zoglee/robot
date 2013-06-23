#ifndef __FILEUTILS_H__
#define __FILEUTILS_H__

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <error.h>
#include <string.h>

#include "singleton.h"


namespace filesystem {

enum TFileError {
	FILE_ERROR_EXIST,
	FILE_ERROR_ISDIR,
	FILE_ERROR_ACCES,
	FILE_ERROR_NAMETOOLONG,
	FILE_ERROR_NOENT,
	FILE_ERROR_NOTDIR,
	FILE_ERROR_NXIO,
	FILE_ERROR_NODEV,
	FILE_ERROR_ROFS,
	FILE_ERROR_TXTBSY,
	FILE_ERROR_FAULT,
	FILE_ERROR_LOOP,
	FILE_ERROR_NOSPC,
	FILE_ERROR_NOMEM,
	FILE_ERROR_MFILE,
	FILE_ERROR_NFILE,
	FILE_ERROR_BADF,
	FILE_ERROR_INVAL,
	FILE_ERROR_PIPE,
	FILE_ERROR_AGAIN,
	FILE_ERROR_INTR,
	FILE_ERROR_IO,
	FILE_ERROR_PERM,
	FILE_ERROR_NOSYS,
	FILE_ERROR_FAILED
};

/* For backward-compat reasons, these are synced to an old 
 * anonymous enum in libgnome. But don't use that enum
 * in new code.
 */
enum TFileTest {
	FILE_TEST_IS_REGULAR    = 1 << 0,
	FILE_TEST_IS_SYMLINK    = 1 << 1,
	FILE_TEST_IS_DIR        = 1 << 2,
	FILE_TEST_IS_EXECUTABLE = 1 << 3,
	FILE_TEST_EXISTS        = 1 << 4
};

#define DIR_SEPARATOR '/'
#define IS_DIR_SEPARATOR(_c) ((_c) == DIR_SEPARATOR)


class FileUtils {
public:
	/**
	 * mkdir_with_parents:
	 * @pathname: log目录 (可以是绝对路径, 也可以是相对路径)
	 * @mode: 创建新目录时会参考这个 mode 的设定; (即: 作为 mkdir的参数)
	 *
	 * 创建目录 (相当于 mkdir -p 的效果)
	 *
	 * Returns:
	 * 	0:  pathname所指的目录已经可用了 (无论是新创建的还是已存在的可写目录)
	 * 	-1: 任何错误发生时返回 -1;
	 */
	int mkdir_with_parents (const char *pathname, int mode);

	/**
	 * @file_test: 测试文件 (stat/access)
	 *
	 * @return
	 * 	0: 测试 OK
	 * 	1: 测试 FAILED
	 */
	int file_test (const char *filename, int test);

private:
	/**
	 * @is_absolute_path: 判断 file_name 是否是绝对路径
	 * @return 0: 不是绝对路径, 1: 是绝对路径
	 */
	int is_absolute_path (const char *file_name);

	/**
	 * @path_skip_root: 返回root后面那一截字符串;
	 *
	 * @return
	 * 	如果 @file_name是个绝对路径, 就返回 '/' 后面的路径字符串;
	 * 	如果 @file_name是个相对路径, 就返回 NULL
	 */
	char *path_skip_root (const char *file_name);
};

typedef singleton_default<FileUtils> FileUtils_Singleton;
#define FILEUTILS (filesystem::FileUtils_Singleton::instance())



} // namespace filesystem


#endif // __FILEUTILS_H__
