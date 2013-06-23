#include "fileutils.h"

namespace filesystem {

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
int FileUtils::mkdir_with_parents (const char *pathname, int mode) {
	char *fn, *p;

	if (pathname == NULL || *pathname == '\0') {
		errno = EINVAL;
		return -1;
	}

	fn = strdup (pathname);

	if (is_absolute_path (fn)) {
		p = (char *) path_skip_root (fn);
	} else {
		p = fn;
	}

	do {
		while (*p && !IS_DIR_SEPARATOR (*p)) p++;

		if (!*p) {
			p = NULL;
		} else {
			*p = '\0';
		}

		if (!file_test (fn, FILE_TEST_EXISTS)) {
			if (mkdir (fn, mode) == -1 && errno != EEXIST) {
				int errno_save = errno;
				free (fn);
				errno = errno_save;
				return -1;
			}
		} else if (!file_test (fn, FILE_TEST_IS_DIR)) {
			free (fn);
			errno = ENOTDIR;
			return -1;
		}

		if (p) {
			*p++ = DIR_SEPARATOR;
			while (*p && IS_DIR_SEPARATOR (*p)) p++;
		}
	} while (p);

	free (fn);

	return 0;
}

/**
 * @is_absolute_path: 判断 file_name 是否是绝对路径
 *
 * @return 0: 不是绝对路径, 1: 是绝对路径
 */
int FileUtils::is_absolute_path (const char *file_name) {
	return IS_DIR_SEPARATOR (file_name[0]) ? 1 : 0;
}

/**
 * @path_skip_root: 返回root后面那一截字符串;
 *
 * @return
 * 	如果 @file_name是个绝对路径, 就返回 '/' 后面的路径字符串;
 * 	如果 @file_name是个相对路径, 就返回 NULL
 */
char *FileUtils::path_skip_root (const char *file_name)
{
  /* Skip initial slashes */
	if (IS_DIR_SEPARATOR (file_name[0])) {
		while (IS_DIR_SEPARATOR (file_name[0])) {
			file_name++;
		}
		return (char *)file_name;
	}

  return NULL;
}

/**
 * @file_test: 测试文件 (stat/access)
 *
 * @return
 * 	0: 测试 OK
 * 	1: 测试 FAILED
 */
int FileUtils::file_test (const char *filename, int test) {
	if ((test & FILE_TEST_EXISTS)
		&& (access (filename, F_OK) == 0)) {
		return 1;
	}

	if ((test & FILE_TEST_IS_EXECUTABLE)
		&& (access (filename, X_OK) == 0)) {
		if (getuid () != 0) { // no root
			return 1;
		}

		/* For root, on some POSIX systems, access (filename, X_OK)
		 * will succeed even if no executable bits are set on the
		 * file. We fall through to a stat test to avoid that.
		 */
	} else {
		test &= ~FILE_TEST_IS_EXECUTABLE;
	}

	if (test & FILE_TEST_IS_SYMLINK) {
		struct stat s;

		if ((lstat (filename, &s) == 0) && S_ISLNK (s.st_mode)) {
			return 1;
		}
	}

	if (test & (FILE_TEST_IS_REGULAR |
				FILE_TEST_IS_DIR |
				FILE_TEST_IS_EXECUTABLE)) {
		struct stat s;

		if (stat (filename, &s) == 0) {
			if ((test & FILE_TEST_IS_REGULAR) && S_ISREG (s.st_mode)) {
				return 1;
			}

			if ((test & FILE_TEST_IS_DIR) && S_ISDIR (s.st_mode)) {
				return 1;
			}

			/* The extra test for root when access (file, X_OK) succeeds.
			*/
			if ((test & FILE_TEST_IS_EXECUTABLE) &&
					((s.st_mode & S_IXOTH) ||
					 (s.st_mode & S_IXUSR) ||
					 (s.st_mode & S_IXGRP))) {
				return 1;
			}
		}
	}

	return 0;
}


} // namespace filesystem
