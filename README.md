# Curses Audio App

## 1 Sizes and Layout

O Tamanho Default é 80 p/24.

Quero uma aleração quando a app arranca para 
120 cols p 24 linhas

## Vistas

```

 <------                     120 col                                      ----->
+------------------------------------------------------------------------------+
|                                                                              |
|                                                                              |
|                  +                                                           |
|                  +                                     +                     |
|          +       +                                     +       +             |
|          +       +                            +        +       +             |
|   +      +       +         +                  +        +       +             |
|   +      +       +         +         +        +        +       +             |
+------------------------------------------------------------------------------+
| 20-50  50-120  120-500   500-1K    1K-2K    2K-5K    5K-10K  10K-20K  (Hz)   |
+------------------------------------------------------------------------------+   24 lines
|                         |                                                    |
|  Time: 1:23 / 5:55      |   Downhome Gril                                    |
|  Vol: 60 /100           |   --------------                                   |
|  Vel: 0.5x              |   ZZ Top - Tres Hombres                            |
|                         |                                                    |
|                         |                                                    |
|                         |                                                    |
|                         |                                                    |
|                         |                                                    |
+------------------------------------------------------------------------------+
|                                    [Q]uit [L]oad [P]lay/[P]ause              |
+------------------------------------------------------------------------------+
```

## Thread Audio vs Thread UI

    Thread Audio                             Thread UI
        |_paCallback()



An audio buffer is a collection of sample frames. A sample frame is a set of audio samples that are coincident in time.