# ASCII Image Converter

Una herramienta escrita en **C++** para convertir imágenes (`JPG`, `PNG`, etc.) en arte ASCII. El programa carga una imagen, la convierte a escala de grises y genera una representación utilizando un conjunto de caracteres seleccionable.

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
* **Póster imprimible en PDF**: la imagen se divide en páginas Carta o A4 numeradas, con los números de las páginas vecinas en los márgenes y una página de resumen con el mapa de ensamblado.
* Varios conjuntos de caracteres predefinidos (`--charset`).
* Código portable y escrito en C++ moderno.

## Funcionamiento

El proceso de conversión consta de los siguientes pasos:

1. Cargar la imagen desde disco.
2. Convertir la imagen a escala de grises.
3. Corregir la relación de aspecto para compensar las proporciones de los caracteres de una fuente monoespaciada.
4. Redimensionar la imagen a las dimensiones solicitadas.
5. Ajustar el brillo y el contraste.
6. Convertir cada píxel a un carácter ASCII según su intensidad.
7. Generar el resultado como texto.

```text
Imagen
   │
   ▼
Escala de grises
   │
   ▼
Corrección de aspecto
   │
   ▼
Redimensionado
   │
   ▼
Brillo / Contraste
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

En la consola el factor es fijo en **2.0** (caracteres el doble de altos que anchos). En la salida PDF se calcula a partir de la fuente Courier: interlineado ÷ ancho de carácter = 1 / 0.6 ≈ **1.67**.

Dimensiones de salida:

* Sin `--width` ni `--height`: ancho igual al de la imagen, con un máximo de **100 columnas**; el alto se calcula a partir de la relación de aspecto.
* Solo `--width`: el alto se calcula automáticamente.
* Solo `--height`: el ancho se calcula automáticamente.
* Ambos: se respetan tal cual (se muestra un aviso si deforman la imagen).
* `--scale s`: `s` caracteres por píxel de la imagen (`1` = un carácter por píxel); sin límite de 100 columnas. No se combina con `--width`/`--height`.

## Tabla de caracteres

Por defecto (`--charset standard`) se utiliza una secuencia ordenada desde los caracteres más oscuros hasta los más claros:

```text
@%#*+=-:. 
```

Con `--charset detailed` se utiliza un conjunto más largo:

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
    --charset detailed \
    --contrast 1.2 \
    --output resultado.txt
```

## Opciones

| Opción         | Descripción              | Valor por defecto |
| -------------- | ------------------------ | ----------------- |
| `--width`      | Ancho del ASCII generado | Automático        |
| `--height`     | Alto del ASCII generado  | Automático        |
| `--scale`      | Caracteres por píxel de la imagen (`1` = 1 px : 1 carácter) | — |
| `--brightness` | Desplazamiento de brillo en % (-100 a 100; 0 = sin cambio) | 0 |
| `--contrast`   | Factor de contraste (≥ 0; 1 = sin cambio, 0 = gris plano) | 1.0 |
| `--output`     | Archivo de salida de texto | Consola         |
| `--charset`    | Conjunto de caracteres   | `standard`        |
| `--invert`     | Invierte la escala (texto claro sobre fondo oscuro) | desactivado |
| `--help`, `-h` | Muestra la ayuda         |                   |

### Póster imprimible (PDF)

Para imágenes grandes, `--pdf` genera un PDF listo para imprimir en varias hojas que luego se recortan y se unen:

```bash
# 1 carácter por píxel, hojas Carta
ascii_converter foto.jpg --pdf poster.pdf

# Exactamente 4 hojas A4 de ancho, con un conjunto de caracteres detallado
ascii_converter foto.jpg --pdf poster.pdf --paper a4 --pages-wide 4 --charset detailed

# Solo calcular cuántas páginas saldrían
ascii_converter foto.jpg --pdf poster.pdf --scale 2 --dry-run
```

| Opción          | Descripción | Valor por defecto |
| --------------- | ----------- | ----------------- |
| `--pdf`         | Archivo PDF de salida. El texto solo se escribe si además se usa `--output` | — |
| `--paper`       | `letter` (Carta) o `a4` | `letter` |
| `--orientation` | `portrait`, `landscape` o `auto` (la que use menos páginas) | `portrait` |
| `--font-size`   | Tamaño de la fuente Courier en puntos | 6 |
| `--pages-wide`  | Escala la imagen para ocupar exactamente *n* páginas de ancho (no se combina con `--scale`) | — |
| `--overlap`     | Caracteres repetidos entre páginas vecinas, para facilitar el pegado | 0 |
| `--glue-flap`   | Pestaña de pegado (en puntos) en los bordes derecho e inferior que se unen a otra página; `0` = sin pestaña. Máximo 36 | 18 (0.25 in ≈ 6 mm) |
| `--max-pages`   | Máximo de páginas; si se supera, no se escribe nada | 50 |
| `--dry-run`     | Muestra el resumen (tamaño y número de páginas) sin escribir el PDF | — |

Sin `--width`, `--height`, `--scale` ni `--pages-wide`, la salida PDF usa **1 carácter por píxel**. Estas opciones de página requieren `--pdf`, y el conjunto `blocks` no está disponible en PDF (solo ASCII).

Contenido del PDF:

* **Página de resumen** (primera): nombre de la imagen, tamaño en caracteres, número de páginas, papel y fuente, instrucciones de ensamblado y un mapa de la cuadrícula con el número de cada página.
* **Páginas de la imagen**, numeradas por filas desde 1:
  * La imagen empieza en la misma posición en todas las hojas, para que encajen al unirlas.
  * **Marcas de alineación** (esquinas continuas): el borde exacto de la imagen, donde debe quedar el borde de la hoja vecina.
  * **Marcas de recorte** (discontinuas): solo en los bordes derecho e inferior que se unen a otra página; indican dónde cortar para dejar una pestaña de pegado de `--glue-flap` puntos.
  * Número de página y posición (`Page 5/9  row 2, col 2`).
  * Número de cada página vecina junto al borde correspondiente: `^` arriba, `v` abajo, `<` izquierda, `>` derecha.

Con cada ejecución se muestra un resumen en `stderr`, por ejemplo:

```text
[SUMMARY] 540x431 chars -> 4x4 = 16 pages + overview (A4 portrait, 6 pt Courier)
```

**Atención al tamaño:** a 1 carácter por píxel, una foto de 4000×3000 píxeles necesita unas 600 hojas Carta a 6 pt. Use `--dry-run` para comprobarlo y `--pages-wide` o `--scale` para ajustar el tamaño.

Ensamblado: corte los bordes izquierdo y superior por las marcas de alineación, y los bordes derecho e inferior por las marcas de recorte (conservando la pestaña). Coloque cada hoja sobre las pestañas de sus vecinas de la izquierda y de arriba, con su borde sobre las marcas de alineación de esas vecinas, y pegue. Con `--glue-flap 0` no hay pestañas: se corta todo por las marcas de alineación y las hojas se unen borde con borde.

Para imprimir, use **tamaño real / 100 %** (sin "ajustar a la página"); si la impresora escala las hojas, las páginas vecinas no encajarán.

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

Los píxeles oscuros producen caracteres más densos, mientras que los claros generan caracteres más ligeros o espacios. Con `--invert` la relación se invierte, lo que suele verse mejor en terminales con fondo oscuro.

Las zonas transparentes (PNG con canal alfa) se tratan como vacías y se representan con espacios, tanto en modo normal como invertido.

## Dependencias

El proyecto puede compilar utilizando cualquier compilador compatible con **C++17** o superior.

Para cargar imágenes se utiliza [stb_image](https://github.com/nothings/stb), incluida en `include/` (no requiere instalación).

## Compilación

Ejemplo utilizando CMake:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### Pruebas

```bash
ctest --test-dir build --output-on-failure
```

Incluye pruebas unitarias de `ImageConverter` (tamaños, escala, conjuntos de caracteres, brillo/contraste, inversión, transparencia), de la paginación (`PageLayout`), del escritor de PDF (`PdfWriter`) y del contenido de las páginas (`PosterRenderer`), además de pruebas de la línea de comandos. Las pruebas de línea de comandos usan la imagen `tests/data/circle.png`.

## Ejemplo de salida

Un círculo con degradado radial (oscuro en el centro), con `--width 40`:

```text


                  ....
            .....::::::.....
          ...:::--------::::..
        ..:::---========---:::..
       ..::--===++++++++===--::...
      ..::--==++********++===--:..
     ..::--==++**######***++=--::..
     ..::-==++**##%%%%%#**++==--:..
     ..::-==++**##%%%%%#**++==--:..
     ..::--==++**######***++=--::..
      ..::--==++********++===--:..
       ..::--===++++++++===--::...
        ..:::---========---:::..
          ...:::--------::::..
            .....::::::.....
                  ....


```

Los mensajes de progreso se escriben en `stderr`; `stdout` contiene solo el arte ASCII, por lo que puede redirigirse:

```bash
ascii_converter foto.png > arte.txt
```

## Posibles mejoras

* Soporte para imágenes a color utilizando códigos ANSI.
* Exportación a HTML.
* Exportación a SVG.
* Generación de animaciones ASCII.
* Conversión de GIF y video.
* Procesamiento en paralelo para imágenes grandes.
* Paletas de caracteres definidas por el usuario (además de los presets).
* Factor de corrección de aspecto configurable en la consola (en PDF ya se calcula a partir de la fuente).
* Compresión de los PDF generados.

## Licencia

Este proyecto puede distribuirse bajo la licencia MIT.
