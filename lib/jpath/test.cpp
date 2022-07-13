/*
** MIT License
**
** For the latest info, see https://github.com/paladin-t/jpath
**
** Copyright (C) 2020 - 2022 Tony Wang
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all
** copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
** SOFTWARE.
*/

#include "jpath.hpp"

int main(int argc, const char* argv[]) {
	rapidjson::Document doc;

	int i32 = 0;
	float real = 0.0f;
	const char* str = nullptr;

	Jpath::set(doc, doc, 42, "hello", 0, "world");
	Jpath::set(doc, doc, 22/7.0f, "hello", 0, "pi");
	Jpath::get(doc, i32, "hello", 0, "world");
	Jpath::get(doc, real, "hello", 0, "pi");
	printf("%d\n", i32);
	printf("%f\n", real);
	Jpath::set(doc, doc, "test", "hello", 0, "world");
	Jpath::get(doc, str, "hello", 0, "world");
	printf("%s\n", str);

	return 0;
}
