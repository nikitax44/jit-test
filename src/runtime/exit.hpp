#pragma once
#include <exception>
class exit_exception : std::exception {
  int _status;

public:
  exit_exception(int status) : _status(status) {}
  int status() const { return _status; }
};
