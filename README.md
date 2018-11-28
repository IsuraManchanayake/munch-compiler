# Munch compiler

This is a compiler design project for munch language (following the tutorial series of [Bitwise](https://www.youtube.com/channel/UCguWV1bZg1QiWbY32vGnOLw))

Look at DIARY.md for updates

## Compile

```
cd munch_compiler
gcc main.c -o munch -O3
```

## Usage

```
./munch src_path [-W-no]
```

Add `-W-no` to disable warnings

## Run

```
./munch munch_test/test1.mch
```

or generate your own source file

```
cd munch_test && python gen_source.py && cd ..
./munch munch_test/test1.mch
```

A wiki will be uploaded soon