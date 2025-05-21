#ifndef __COMMON_H__
#define __COMMON_H__

// STL include files
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <queue>
#include <string>
#include <map>
#include <vector>
#include <set>
#include <list>
#include <memory>
#include <algorithm>

// protobuf common include files
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/dynamic_message.h>

// google modules include files
#include <glog/logging.h>
#include <gflags/gflags.h>

// other include files
#include "singleton.h"

// c include files
extern "C" {
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <dirent.h>
#include <fnmatch.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <glib.h>
}

// STL include files
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <queue>
#include <string>
#include <map>
#include <vector>
#include <set>
#include <list>
#include <memory>
#include <algorithm>

// protobuf common include files
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/dynamic_message.h>

// google modules include files
#include <glog/logging.h>
#include <gflags/gflags.h>

// other include files
#include "singleton.h"


#define DETAIL	(2)
#define VERBOSE	(3)

#define FOREACH(container, it) \
	for(typeof((container).begin()) it=(container).begin(); it!=(container).end(); ++it)

#define REVERSE_FOREACH(container, it) \
	for(typeof((container).rbegin()) it=(container).rbegin(); it!=(container).rend(); ++it)

#define FOREACH_NOINCR_ITER(container, it) \
	for(typeof((container).begin()) it=(container).begin(); it!=(container).end();) 

#define REVERSE_FOREACH_NOINCR_ITER(container, it) \
	for(typeof((container).rbegin()) it=(container).rbegin(); it!=(container).rend();)

#define GLIB_INT_TO_POINTER(i)  ((gpointer) (glong) (i))
#define GLIB_POINTER_TO_INT(p)  ((gint)  (glong) (p))

const std::string kNilStr = "NIL";


#endif // __COMMON_H__
