Simple logger for C++.

* Licence: MIT.
* Thread-safe.
* Simple and fast.
* Linux only (it is easy to port to another system).

Usage:

```cpp
   logInfo() << "string" << "to" << "log" << 10;
   logInfo().nospace() << "string" << "to" << "log" << 10;
   logInfo().quote() << "string" << "to" << "log" << 10;
```

Ouput:
```
  03.08.2017 12:44:15.737 I [26629] : string to log 10
  03.08.2017 12:44:15.737 I [26629] : stringtolog10
  03.08.2017 12:44:15.737 I [26629] : "string" "to" "log" "10"
```

For custom types you must privide `std::to_string` overload.
