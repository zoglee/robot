#ifndef __PB_MASTER_H__
#define __PB_MASTER_H__

// c include files
extern "C" {
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <dirent.h>
#include <libgen.h>
}

// STL include files
#include <memory>

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

// other include files
#include "singleton.h"


namespace protobuf_master {

using google::protobuf::Message;
using google::protobuf::FieldDescriptor;
using google::protobuf::compiler::Importer;
using google::protobuf::compiler::DiskSourceTree;
using google::protobuf::FileDescriptor;
using google::protobuf::compiler::MultiFileErrorCollector;


class PbConfErrorCollector : public MultiFileErrorCollector {
public:
	~PbConfErrorCollector() {}
	PbConfErrorCollector() {}

	// implements ErrorCollector ---------------------------------------
	void AddError(const std::string &filename, int line,
			int column, const std::string &errmsg) {
		LOG(ERROR) << "Error: failed to import, file: " << filename
			<< "(" << line << ":" << column << "): " << errmsg;
	}
};

class PbMaster {
public:
	~PbMaster() { }
	PbMaster() : importer_(&src_tree_, &err_collector_) { }

public: // interface
	// @return: 0: failed, !0: message
	Message *create_message(const std::string &type_name);
	// @return: NULL: failed
	Message *make_text_format_message(const std::string &data_fullpath,
									  const std::string &type_name);
	// @return: -1: failed, 0: succ
	int load_text_format_string_message(const std::string text_format_string,
										Message *message);
	// @return: -1: failed, 0: succ
	int load_text_format_message(const std::string &data_fullpath, Message *message);
	// @return: NULL: failed
	const FileDescriptor *import(const std::string &proto_dir,
								 const std::string &proto_file_basename);
	// @return false: failed
	bool import_file(const std::string &proto_file);
	// @return false: failed
	bool import_dir(const std::string &proto_dir);
	// @return false: failed
	bool import_path(const std::string &proto_path);
	// @return: NULL: failed
	const FileDescriptor *get_proto_file_descriptor(const std::string &proto_file_basename);
	// @return: NULL: failed
	const FieldDescriptor *get_message_field_descriptor(Message *message, const std::string &field_name);


private:
	Importer importer_;
	PbConfErrorCollector err_collector_;
	DiskSourceTree src_tree_;
};
typedef singleton_default<PbMaster> PbMaster_Singleton;
#define PB_MASTER (protobuf_master::PbMaster_Singleton::instance())




} // namespace protobuf_master

#endif // __PB_MASTER_H__
