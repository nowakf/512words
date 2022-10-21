local ffi = require 'ffi'

local path = ...

local header = assert(io.open(path .. '/512_words.h')):read('*a')

ffi.cdef(header)

local libwords = ffi.load(path .. '/lib512.so')

function to_closest_utf8(filename, block_len)
	local file = assert(io.open(filename, 'rb'))
	local buf     = ffi.new('char [?]', block_len)
	local changes = ffi.new('char [?]', block_len)
	while true do
		local bytes = file:read(block_len)
		if not bytes then break end
		ffi.fill(buf, block_len) 
		ffi.fill(changes, block_len) 	--clear
		ffi.copy(buf, bytes)	 
		ffi.copy(changes, bytes)	 --copy
		libwords.to_utf8(buf, block_len)
		libwords.cut_invalid_control_chars(buf, block_len)
		libwords.diff(changes, buf, block_len)
		print(ffi.string(buf, block_len))
	end
end
