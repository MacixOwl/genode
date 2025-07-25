/*
 * \brief  File system for providing a read-only value as a file
 * \author Norman Feske
 * \date   2018-03-27
 */

/*
 * Copyright (C) 2018 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__VFS__READONLY_VALUE_FILE_SYSTEM_H_
#define _INCLUDE__VFS__READONLY_VALUE_FILE_SYSTEM_H_

/* Genode includes */
#include <util/xml_generator.h>
#include <vfs/single_file_system.h>

namespace Vfs {
	template <typename, unsigned BUF_SIZE = 128>
	class Readonly_value_file_system;
}


template <typename T, unsigned BUF_SIZE>
class Vfs::Readonly_value_file_system : public Vfs::Single_file_system
{
	public:

		using Name = Genode::String<64>;

	private:

		using Buffer = Genode::String<BUF_SIZE + 1>;

		Name const _file_name;

		Buffer _buffer { };

		struct Vfs_handle : Single_vfs_handle
		{
			Buffer const &_buffer;

			Vfs_handle(Directory_service &ds,
			           File_io_service   &fs,
			           Allocator         &alloc,
			           Buffer      const &buffer)
			:
				Single_vfs_handle(ds, fs, alloc, 0), _buffer(buffer)
			{ }

			Read_result read(Byte_range_ptr const &dst, size_t &out_count) override
			{
				out_count = 0;

				if (seek() > _buffer.length())
					return READ_ERR_INVALID;

				char const * const src = _buffer.string() + seek();
				size_t const len = min(size_t(_buffer.length() - seek()), dst.num_bytes);
				Genode::memcpy(dst.start, src, len);

				out_count = len;
				return READ_OK;
			}

			Write_result write(Const_byte_range_ptr const &, size_t &) override
			{
				return WRITE_ERR_IO;
			}

			bool read_ready()  const override { return true; }
			bool write_ready() const override { return false; }
		};

		using Config = Genode::String<200>;
		Config _config(Name const &name) const
		{
			char buf[Config::capacity()] { };
			Genode::Xml_generator::generate({ buf, sizeof(buf) }, type_name(),
				[&] (Genode::Xml_generator &xml) { xml.attribute("name", name); }
			).with_error([&] (Genode::Buffer_error) {
				warning("VFS read-only value fs config failed (", _file_name, ")");
			});
			return Config(Genode::Cstring(buf));
		}

		using Registered_watch_handle = Genode::Registered<Vfs_watch_handle>;
		using Watch_handle_registry   = Genode::Registry<Registered_watch_handle>;

		Watch_handle_registry _handle_registry { };

	public:

		Readonly_value_file_system(Name const &name, T const &initial_value)
		:
			Single_file_system(Node_type::TRANSACTIONAL_FILE, type(),
			                   Node_rwx::ro(), Xml_node(_config(name).string())),
			_file_name(name)
		{
			value(initial_value);
		}

		static char const *type_name() { return "readonly_value"; }

		char const *type() override { return type_name(); }

		void value(T const &value)
		{
			_buffer = Buffer(value);

			_handle_registry.for_each([] (Registered_watch_handle &handle) {
				handle.watch_response(); });
		}

		bool matches(Xml_node const &node) const
		{
			return node.has_type(type_name()) &&
			       node.attribute_value("name", Name()) == _file_name;
		}


		/*********************************
		 ** Directory-service interface **
		 *********************************/

		Open_result open(char const  *path, unsigned,
		                 Vfs::Vfs_handle **out_handle,
		                 Allocator   &alloc) override
		{
			if (!_single_file(path))
				return OPEN_ERR_UNACCESSIBLE;

			try {
				*out_handle = new (alloc)
					Vfs_handle(*this, *this, alloc, _buffer);

				return OPEN_OK;
			}
			catch (Genode::Out_of_ram)  { return OPEN_ERR_OUT_OF_RAM; }
			catch (Genode::Out_of_caps) { return OPEN_ERR_OUT_OF_CAPS; }
		}

		Stat_result stat(char const *path, Stat &out) override
		{
			Stat_result result = Single_file_system::stat(path, out);
			out.size = _buffer.length();
			return result;
		}

		Watch_result watch(char const        *path,
		                   Vfs_watch_handle **handle,
		                   Allocator         &alloc) override
		{
			if (!_single_file(path))
				return WATCH_ERR_UNACCESSIBLE;

			try {
				*handle = new (alloc)
					Registered_watch_handle(_handle_registry, *this, alloc);

				return WATCH_OK;
			}
			catch (Genode::Out_of_ram)  { return WATCH_ERR_OUT_OF_RAM;  }
			catch (Genode::Out_of_caps) { return WATCH_ERR_OUT_OF_CAPS; }
		}

		using Single_file_system::close;

		void close(Vfs_watch_handle *handle) override
		{
			Genode::destroy(handle->alloc(),
			                static_cast<Registered_watch_handle *>(handle));
		}
};

#endif /* _INCLUDE__VFS__READONLY_VALUE_FILE_SYSTEM_H_ */
