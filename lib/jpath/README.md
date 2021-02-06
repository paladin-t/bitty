# Jpath - An easy to use RapidJSON reader/writer

Jpath can be used to read and write a RapidJSON value in an intuitive way.

### Dependency

Requires a C++ compiler with variadic template (since C++11) capability.

### Usage

* `Jpath::get(val, ref /* out */, ... /* path */)`: reads `ref` from `val` along with the specific path; fails if any intermediate node does not exist
	* returns `true` for success, `false` for fail
* `Jpath::set(doc, val, ref /* in */, ... /* path */)`: writes `ref` to `val` along with the specific path; omitted intermediate nodes are generated
	* returns `true` for success, `false` for fail

### Example

```cpp
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
```

The above code manipulates a JSON as:

```json
{
  "hello": [
    {
      "world": ...,
      "pi": ...
    }
  ]
}
```
