local ffi = require 'ffi'

local path = ...

local header = assert(io.open(path .. '/512_words.h')):read('*a') ..
[[
void free(void *ptr);
]]

ffi.cdef(header)

local libwords = ffi.load(path .. '/lib512.so')

local image = {
	w = 0,
	h = 0,
	rg8 = nil,
	utf8 = nil,
	bits = nil,
}

function threshold(bytes, threshold, len)
	local bits = ffi.new('uint8_t[?]', len/8)
	libwords.threshold(bytes, bits, len, threshold)
	for i=0,len/8 do
		print(bits[i])
	end
	return bits
end

function image:to_string()
	return ffi.string(self.utf8, self.w * self.h / 8)
end

function image:diff()
	return ffi.gc(libwords.diff(self.bits, self.utf8, self.w*self.h / 8), ffi.C.free)
end

function image.new(bytes, w, h)
	assert(bytes and w and h)

	libwords.rgba2luma(bytes, w*h*4)

	local cw = w - (w%8)
	local ch = h
	local thr = 40 --think about this

	libwords.crop(bytes, w, cw, ch)

	local len = cw * ch
	
	local bits = threshold(bytes, thr, len)

	local utf8 = ffi.gc(libwords.to_utf8(bits, len/8), ffi.C.free)

	local composite = ffi.new('uint8_t[?]', len*2)

	libwords.composite(composite, bits, utf8, len)

	return setmetatable({
		w = cw,
		h = ch,
		rg8 = composite,
		utf8 = utf8,
		bits = bits,
	}, {__index = image})
end

return image
