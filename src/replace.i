%module replace 

%{
#include "replace_external.h"
%}

%include <stl.i>
%include <typemaps.i>
%include <std_string.i>
%include <std_vector.i>
%include <std_pair.i>

%include "replace_external.h"
