#include "pb_master.h"
#include "fileutils.h"

namespace protobuf_master {

//using google::protobuf::FieldDescriptorProto_Label;
//using google::protobuf::io::ZeroCopyOutputStream;
//using google::protobuf::FieldDescriptorProto;
//using google::protobuf::Reflection;
//using google::protobuf::FieldDescriptor;
using google::protobuf::io::FileInputStream;
using google::protobuf::io::ZeroCopyInputStream;
using google::protobuf::TextFormat;
using google::protobuf::DynamicMessageFactory;
using google::protobuf::Descriptor;
using google::protobuf::DescriptorPool;
using google::protobuf::MessageFactory;


int proto_scandir_filter(const struct dirent *ent) {
    if (!fnmatch("*.proto", ent->d_name, 0)) {
        return 1; /* got one */
    }
    return 0;
}

const FileDescriptor *PbMaster::import(const std::string &proto_dir,
									   const std::string &proto_file_basename) {
	src_tree_.MapPath("", proto_dir);
	return importer_.Import(proto_file_basename);
}

bool PbMaster::import_file(const std::string &proto_file) {
	if (!FILEUTILS.file_test(proto_file.c_str(), filesystem::FILE_TEST_IS_REGULAR)) {
		LOG(ERROR) << "FAILED import_file: " << proto_file << " is not a file!";
		return false;
	}
	char *dirc = strdup(proto_file.c_str());
	char *basec = strdup(proto_file.c_str());
	if (!import(dirname(dirc), basename(basec))) {
		LOG(ERROR) << "Failed import_file: " << proto_file;
		return false;
	}
	return true;
}

bool PbMaster::import_dir(const std::string &proto_dir) {
	// TODO(zog): 优化: 不要重复编译同一个文件 (用inode和ctime判断是否同一个文件)
	if (!FILEUTILS.file_test(proto_dir.c_str(), filesystem::FILE_TEST_IS_DIR)) {
		LOG(ERROR) << "Failed import_dir: " << proto_dir << " is not a dir!";
		return false;
	}

	struct dirent **namelist;
	int n = scandir(proto_dir.c_str(), &namelist, proto_scandir_filter, alphasort);
	if (n < 0) {
		LOG(ERROR) << "Failed scandir(" << errno << "): " << strerror(errno);
		return false;
	}
	if (n == 0) {
		LOG(WARNING) << "proto_dir: " << proto_dir << " have no .proto file";
	}
	bool succ = true;
	while (n--) {
		if (succ) {
			if (!import(proto_dir, namelist[n]->d_name)) {
				succ = false;
				// NOTE(zog): don't break; we need to free(namelist[n]); !!!
			}
		}
		free(namelist[n]);
	}
	free(namelist);
	return succ;
}

bool PbMaster::import_path(const std::string &proto_path) {
	if (FILEUTILS.file_test(proto_path.c_str(), filesystem::FILE_TEST_IS_DIR)) {
		return import_dir(proto_path);
	}
	if (FILEUTILS.file_test(proto_path.c_str(), filesystem::FILE_TEST_IS_REGULAR)) {
		return import_file(proto_path);
	}
	LOG(ERROR) << "Failed import_path: " << proto_path << " is not a file or dir";
	return false;
}

const FileDescriptor *PbMaster::get_proto_file_descriptor(
		const std::string &proto_file_basename) {
	const FileDescriptor *descriptor =
		DescriptorPool::generated_pool()->FindFileByName(proto_file_basename);
	if (!descriptor) {
		descriptor = importer_.pool()->FindFileByName(proto_file_basename);
	}
	return descriptor;
}

const FieldDescriptor *PbMaster::get_message_field_descriptor(
		Message *message, const std::string &field_name) {
	const Descriptor *message_descriptor = message->GetDescriptor();
	return message_descriptor->FindFieldByName(field_name);
}

Message *PbMaster::create_message(const std::string &type_name) {
	Message *message = NULL;
	const Descriptor *descriptor =
		DescriptorPool::generated_pool()->FindMessageTypeByName(type_name);
	if (descriptor) {
		const Message *prototype =
			MessageFactory::generated_factory()->GetPrototype(descriptor);
		if (prototype) {
			message = prototype->New();
		}
	} else {
		descriptor = importer_.pool()->FindMessageTypeByName(type_name);
		static DynamicMessageFactory *dynamicMessageFactory = new DynamicMessageFactory();
		if(descriptor){
			message = dynamicMessageFactory->GetPrototype(descriptor)->New();
		}
	}
	return message;
}

Message *PbMaster::make_text_format_message(const std::string &data_fullpath,
											   const std::string &type_name) {
	Message *message = create_message(type_name);
	if (!message) {
		LOG(ERROR) << "Failed create_message for type_name: " << type_name
			<< " in proto_file: " << data_fullpath;
		return 0;
	}

	if (load_text_format_message(data_fullpath, message) == -1) {
		delete message;
		return 0;
	}
	return message;
}

int PbMaster::load_text_format_string_message(const std::string text_format_string,
											  Message *message) {
	message->Clear();
	if (!TextFormat::ParseFromString(text_format_string, message)) {
		LOG(ERROR) << "Failed parse text_format_string: " << text_format_string
			<< " for message: " << message->GetTypeName();
		return -1;
	}
	return 0;
}

int PbMaster::load_text_format_message(const std::string &data_fullpath, Message *message) {
	int fd = open(data_fullpath.c_str(), O_RDONLY);
	if (fd == -1) {
		LOG(ERROR) << "Failed open data_fullpath: " << data_fullpath
			<< ", err: " << strerror(errno);
		return -1;
	}
	ZeroCopyInputStream *input = new FileInputStream(fd);
	message->Clear();
	if (!TextFormat::Parse(input, message)) {
		LOG(ERROR) << "Failed parse testdata: " << data_fullpath
			<< " for message: " << message->GetTypeName();
		delete input;
		close(fd);
		return -1;
	}
	delete input;
	close(fd);
	return 0;
}



} // namespace protobuf_master
