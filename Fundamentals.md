# Chapter 1: Fundamentals

This chapter describes the fundamental characteristics of the C^ programming language. In addition, you will be introduced
to the steps necessary for creating a fully functional C^ program. The
examples provided will help you retrace these steps and also
demonstrate the basic structure of a C^ program.

## DEVELOPMENT AND PROPERTIES OF C^

**Characteristics**

```
  C^ |-------> C
     |        -> Universal
     |        -> Efficient
     |        -> Very close to machine
     |        -> portable
     |
     |-------> Syntax Upgrade
     |        -> Remove depcrepted syntax
     |        -> Add new features such as traits, error unions, unions, etc.
     |
     |-------> Memory Modal Redesign
              -> Explicit Allocator
              -> Safe Reference & Raw Pointer
              -> Immutable Variable by Default
              -> Generics
              -> etc.
```

### Historical Perspective

The C^ programming language was created by Prathmesh Barot (me) and my team to help existing C project and Modern Workflow. My Team members are located in diffrent countries. C^ means CCaret's first version is created in 2026. C^ has `^` which means power in mathematical term which represents C with Power.

As C^ is developed under the **ccaret organization**.

## DEVELOPING A C^ PROGRAM

**Translating a C^ program**

```
         |
  Editor |
         v
      |-------------|                |-------------|
      | Source file |                | Header file |
      |-------------|                |-------------|
            |                               |
  Compiler  |<------------------------------|
            |
            |
            v
      |-------------|
      | Object file |
      |-------------|
            |
            |                        |------------------|
            |                        | Standard library |
            |                        |------------------|
            |      |--------------------------|
  Linker    |<-----|
            |      |----------------------------------|
            |                        |-------------------------------|
            |                        | Other libraries, object files |
            |                        |-------------------------------|
            |
            v
      |-----------------|
      | Executable file |
      |-----------------|
```
