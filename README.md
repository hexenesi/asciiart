# ASCII Image Converter

Una herramienta escrita en **C++** para convertir imágenes (`JPG`, `PNG`, etc.) en arte ASCII. El programa carga una imagen, la convierte a escala de grises y genera una representación utilizando un conjunto configurable de caracteres ASCII.

## Características

* Soporte para imágenes en formato **JPG** y **PNG**.
* Conversión automática a **escala de grises**.
* Generación de arte ASCII utilizando una tabla de caracteres ordenados por densidad.
* Tamaño de salida configurable:

  * Ancho.
  * Alto.
* Ajustes de imagen:

  * Brillo.
  * Contraste.
* Salida por consola o archivo de texto.
* Código portable y escrito en C++ moderno.

## Funcionamiento

El proceso de conversión consta de los siguientes pasos:

1. Cargar la imagen desde disco.
2. Convertir la imagen a escala de grises.
3. Ajustar el brillo y el contraste.
4. Corregir la relación de aspecto para compensar las proporciones de los caracteres de una fuente monoespaciada.
5. Redimensionar la imagen a las dimensiones solicitadas.
6. Convertir cada píxel a un carácter ASCII según su intensidad.
7. Generar el resultado como texto.

```text
Imagen
   │
   ▼
Escala de grises
   │
   ▼
Brillo / Contraste
   │
   ▼
Corrección de aspecto
   │
   ▼
Redimensionado
   │
   ▼
Mapeo de intensidad
   │
   ▼
Arte ASCII
```

### Corrección de la relación de aspecto

Los caracteres de una fuente monoespaciada no son cuadrados: normalmente son más altos que anchos. Si la imagen se redimensiona utilizando únicamente las dimensiones solicitadas, el resultado suele verse estirado verticalmente.

Para evitar esta distorsión, el programa aplica un **factor de corrección de aspecto** antes del redimensionado. Dicho factor ajusta automáticamente la altura (o el ancho, según la configuración) para compensar las proporciones reales de los caracteres.

Por ejemplo, si los caracteres tienen aproximadamente una relación de aspecto de **1:2** (ancho:alto), una imagen de **200×200 píxeles** podría convertirse internamente a **200×100** antes del mapeo ASCII, produciendo una representación visual mucho más fiel al original.

El factor de corrección será configurable para adaptarse a diferentes fuentes monoespaciadas y terminales, permitiendo obtener resultados consistentes independientemente del entorno de visualización.

## Tabla de caracteres

Por defecto se utiliza una secuencia ordenada desde los caracteres más oscuros hasta los más claros:

```text
@%#*+=-:. 
```

También pueden utilizarse conjuntos más largos, por ejemplo:

```text
$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\|()1{}[]?-_+~<>i!lI;:,"^`'.
```

## Uso

```bash
ascii_converter [opciones] <imagen>
```

### Ejemplo

```bash
ascii_converter foto.jpg --width 120 --height 60
```

o

```bash
ascii_converter foto.png \
    --width 160 \
    --height 80 \
    --brightness 15 \
    --contrast 1.2 \
    --output resultado.txt
```

## Opciones

| Opción         | Descripción              | Valor por defecto |
| -------------- | ------------------------ | ----------------- |
| `--width`      | Ancho del ASCII generado | Automático        |
| `--height`     | Alto del ASCII generado  | Automático        |
| `--brightness` | Desplazamiento de brillo en % (-100 a 100; 0 = sin cambio) | 0 |
| `--contrast`   | Factor de contraste (≥ 0; 1 = sin cambio, 0 = gris plano) | 1.0 |
| `--output`     | Archivo de salida        | Consola           |
| `--charset`    | Conjunto de caracteres   | `standard`        |

### Conjuntos de caracteres (`--charset`)

| Nombre     | Caracteres (oscuro → claro)  |
| ---------- | ---------------------------- |
| `standard` | `@%#*+=-:. ` (10 niveles)    |
| `detailed` | Rampa de 70 niveles de Paul Bourke |
| `simple`   | `#+-. `                      |
| `binary`   | `# `                         |
| `blocks`   | `█▓▒░ ` (Unicode, requiere terminal UTF-8) |

## Algoritmo

Cada píxel de la imagen se representa mediante un carácter cuya "densidad visual" corresponde a la intensidad del píxel.

Por ejemplo:

```text
Negro ---------------------------- Blanco

@
%
#
*
+
=
-
:
.
(especial: espacio)
```

Los píxeles oscuros producen caracteres más densos, mientras que los claros generan caracteres más ligeros o espacios.

## Dependencias

El proyecto puede compilar utilizando cualquier compilador compatible con **C++17** o superior.

Para cargar imágenes puede utilizarse cualquiera de las siguientes bibliotecas:

* stb_image (recomendada)
* OpenCV
* FreeImage

## Compilación

Ejemplo utilizando CMake:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Ejemplo de salida

```text
@@@@@@@@@@@@%%%###***++===---::..
@@@@@@@%%%###***+++===---:::....
@@@%%%###***+++===---:::.....
%%###***+++===---:::.......
```

## Posibles mejoras

* Soporte para imágenes a color utilizando códigos ANSI.
* Exportación a HTML.
* Exportación a SVG.
* Generación de animaciones ASCII.
* Conversión de GIF y video.
* Procesamiento en paralelo para imágenes grandes.
* Paletas de caracteres personalizadas.
* Corrección automática de la relación de aspecto de los caracteres.

## Licencia

Este proyecto puede distribuirse bajo la licencia MIT.
