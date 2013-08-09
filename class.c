// declaration

class some_class {
  some_method(arg_t some_arg);
};
struct some_class {
  some_type_t some_member;
};

// preproces to

some_class_some_method(some_class* self, arg_t some_arg);
struct some_class {
  some_type_t some_member;
};

// usage

#include <string.h>

int main() {
  struct some_class* an_intense;
  memset(an_intense, 0, sizeof *an_intense);
  an_intense.some_method(arg);
}

// nor usage

#include <string.h>

int main() {
  struct some_class* an_intense;
  memset(an_intense, 0, sizeof *an_intense);
  some_class_some_method(an_intense, arg);
  // or
  some_method(an_intense, arg);// have problem...
}
