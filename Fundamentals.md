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
     |        -> Remove deprecated syntax
     |        -> Add new features such as traits, error unions, unions, etc.
     |
     |-------> Memory Model Redesign
              -> Explicit Allocator
              -> Safe Reference & Raw Pointer
              -> Immutable Variable by Default
              -> Generics
              -> etc.
```

### Historical Perspective

The C^ programming language was created by Prathmesh Barot (me) and my team to help existing C projects and modern workflows. My team members are located in different countries. C^ means CCaret's first version.

As C^ is developed under the **ccaret organization**.

## DEVELOPING A C^ PROGRAM

**Translating a C^ program**

```
                           ┌─────────────┐
                           │   Editor    │
                           └──────┬──────┘
                                  │
                    ┌─────────────┴──────────────┐
                    │                            │
            ┌───────▼────────┐          ┌────────▼──────┐
            │  Source File   │          │  Header File  │
            └───────┬────────┘          └────────┬──────┘
                    │                            │
                    └─────────────┬──────────────┘
                                  │
                    ┌─────────────▼──────────────┐
                    │      Compiler             │
                    └─────────────┬──────────────┘
                                  │
                    ┌─────────────▼──────────────┐
                    │   Object File             │
                    └─────────────┬──────────────┘
                                  │
                    ┌─────────────┴────────────────────────┐
                    │                                      │
        ┌───────────▼────────────┐      ┌─────────────────▼──────┐
        │  Standard Library      │      │ Other Libraries &      │
        │  Symbols              │      │ Object Files           │
        └────────────────────────┘      └────────────────────────┘
                    │                                      │
                    └─────────────┬──────────────────────┘
                                  │
                    ┌─────────────▼──────────────┐
                    │      Linker               │
                    └─────────────┬──────────────┘
                                  │
                    ┌─────────────▼──────────────┐
                    │  Executable File          │
                    └───────────────────────────┘
```
