# carista-task

Interview task for Carista

## Description

Simple CAN frame parser based on https://en.wikipedia.org/wiki/ISO_15765-2

## Getting Started

### Dependencies

* Linux based OS
* C++ compiler with C++20 support (i.e. g++13)
* cmake

### Build

```
cd carista-task
mkdir build && cd build
cmake ..
make
```

### Executing the program

```
./carista-task <path_to_input_file>
```

```
./carista-task ../input.txt
```

### Build unit tests

```
cd carista-task
mkdir build && cd build
cmake ..
make test
```

### Executing unit tests

```
./test
```

## Authors

Pavel Vasilev

## License

This project is licensed under the MIT License - see the LICENSE.md file for details