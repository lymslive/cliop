# Next to do

* config read enhance, eg. escape quote
* error optimization

```cpp
    ERROR_CODE_CONFIG_INVALID,     //< can not read config file
    ERROR_CODE_ARGUMENT_INVALID,   //< argument may invalid
    ERROR_CODE_OPTION_EMPTY,       //< option long name is empty
    ERROR_CODE_OPTION_REDEFINE,    //< option name is redefined
    ERROR_CODE_FLAG_INVALID,       //< flag letter invalid
    ERROR_CODE_FLAG_REDEFINE,      //< flag letter is redefined
```

CatchError(int nError);
