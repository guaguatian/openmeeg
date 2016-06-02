// DESCRIPTION
//
//   Test to check whether the compiler supports basic templates.
//   In sync with AC_CXX_TEMPLATES (2008-04-12).
//
// COPYLEFT
//
//   Copyright (c) 2008 Todd Veldhuizen
//   Copyright (c) 2009 Theo Papadopoulo <Theodore.Papadopoulo@inria.fr>
//   Copyright (c) 2008 Luc Maisonobe <luc@spaceroots.org>
//
//   Copying and distribution of this file, with or without modification, are
//   permitted in any medium without royalty provided the copyright notice
//   and this notice are preserved.

template<class T> class A {
public:
    A() { }
};

template<class T> void f(const A<T>&) { }

int main() {
    A<double> d;
    A<int> i;
    f(d);
    f(i);
    return 0;
}
