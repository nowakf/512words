local ffi = require 'ffi'

local path = ...

local header = assert(io.open(path .. '/512_words.h')):read('*a') ..

[[ 
void free(void *ptr);
]]

ffi.cdef(header)

local libwords = ffi.load(path .. '/lib512.so')

local image = {}

local formats = {
	rgba8 = 4,
	r8 = 1,
	r1 = 1/8,
}

function image:crop(w, h)
	assert(self.w >= w and self.h >= h)
	self.w = w
	self.h = h
	libwords.crop(self.luma, self.w, w, h)
end


function image:quarter()
	libwords.quarter(self.luma, self.w, self.h)
	self.w = self.w / 2
	self.h = self.h / 2
end

function image:to_string()
	local utf8 = self:to_utf8()
	local str = ffi.string(utf8, self:len_bytes())
	return str
end

function image:to_utf8()
	local utf8 = libwords.to_utf8(self.luma, self:len_bytes())
	return ffi.gc(utf8, ffi.C.free)
end

function image:len_bytes()
	return self.w * self.h * formats[self.format]
end

function image:to_diff()
	local utf8 = self.utf8 or self:to_utf8()
	local diff = libwords.diff(self.luma, self.utf8, self:len_bytes())
	return ffi.gc(diff, ffi.C.free)
end

function image:desaturate()
	if self.format ~= 'rgba8' then error('image is already desaturated') end
	libwords.rgba2luma(self.luma, self:len_bytes())
	self.format = 'r8'
end

function image:threshold(threshold)
	if self.format ~= 'r8' then error('make image desaturated first') end
	libwords.threshold(self.luma, self:len_bytes(), threshold)
	self.format = 'r1'
end

function image:to_png()
	if self.format ~= 'r8' then error("make image bitmap first") end
	return bitmap_to_png(self.luma, self.w, self.h)
end

function bitmap_to_png(bytes, w, h)
	local png = ffi.new('uint8_t *')
	local size = ffi.new('uint8_t[1]')
	libwords.encode_png(png, size, bytes, w, h)
	return ffi.gc(png, ffi.C.free)
end

function image.new(bytes, w, h, format)
	assert(bytes and w and h)

	return setmetatable({
		w = w,
		h = h,
		luma = bytes,
		format = format or 'rgba8'
	}, {__index = image})
end

return image
