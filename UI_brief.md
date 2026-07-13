# La Distorneta — Brief de UI

Redactado con un subagente Fable 5, a pedido de Hernán, como brief creativo para diseñar la UI del plugin en Claude Design y generar los assets de jugadores en Nano Banana.

## 1. Brief para Claude Design

Necesito un mockup HTML standalone (CSS y JS embebidos, para WebView de JUCE) de la UI de un plugin VST de distorsión llamado **LA DISTORNETA**, hermano de "MySaturator – La Scaloneta". Misma familia visual albiceleste, pero versión más agresiva y rockera: menos suavidad, más garra.

**Layout (ventana ~760×520px, escalable):**

- **Header:** título "LA DISTORNETA" grande, centrado, tipografía display condensada bold (tipo Anton, Bebas Neue o similar de camiseta de fútbol), blanco con contorno o sombra celeste, tres estrellas doradas arriba. Subtítulo chico: "Distorsión Campeona del Mundo".
- **Fila de jugadores (bajo el header):** 5 botones-tarjeta horizontales para elegir el tipo de distorsión. Cada uno con espacio circular (~90px) para la cara ilustrada del jugador, nombre abajo y etiqueta chica del algoritmo: MESSI (Válvula), DIBU (Fuzz), JULIÁN (Overdrive), LAUTARO (Hard Clip), DE PAUL (Bit Crush). Estado activo: borde dorado brillante y leve glow; inactivos, desaturados.
- **Zona central:** 4 knobs rotativos grandes (~80px): Drive, Tone, Mix, Output. Estilo metálico oscuro con indicador celeste y arco de valor dorado. Labels en mayúsculas.
- **Botón GARRA:** toggle destacado a la derecha de los knobs, estilo interruptor industrial o botón de arcade rojo/dorado. Encendido: glow fuego/dorado y texto "GARRA" iluminado. Es el "modo bestia": que se note.
- **VU meter (abajo o lateral derecho):** dos diales analógicos de aguja (L y R), fondo marfil/hueso (#F5EFD8), marcas de escala negras (-20 a +3 dB), zona roja sobre 0dB, aguja roja con pivote inferior. En JS: animar la aguja con inercia/suavizado (lerp), nada de saltos bruscos. Simular movimiento con audio fake para el demo.

**Paleta:** fondo azul noche (#0D1B3D / #101F4A), celeste selección (#75AADB), blanco (#F5F5F5), dorado campeón (#D4A017 / #FFD700) para acentos y estados activos, rojo aguja (#C0392B), y toques de textura oscura (carbono, metal cepillado) para el costado rockero.

**Mood:** agresivo pero canchero, con humor futbolero post-Mundial. Que parezca un pedal de distorsión tuneado por la banda de la Selección. Detalles bienvenidos: sutil trama de escudo/bandera de fondo, tornillos en las esquinas, desgaste tipo pedalera. Interpretación creativa libre mientras la jerarquía sea: jugadores → knobs → Garra → VU.

## 2. Prompts para Nano Banana (íconos de jugadores)

**Estilo base común (agregar a cada uno):** *"Stylized flat vector illustration portrait, head and shoulders, bold outlines, warm colors, Argentina national team jersey (sky blue and white stripes), dark navy background, sticker/badge style, clean and readable at small icon size, no text, not photorealistic."*

1. **Messi:** "Illustrated portrait of a legendary Argentine footballer, short beard, calm confident smile, golden subtle glow behind him, number 10 vibe."
2. **Dibu Martínez:** "Illustrated portrait of a tall Argentine goalkeeper, intense wild grin, expressive eyes, slicked-back hair, mischievous troublemaker energy, green goalkeeper jersey accent."
3. **Julián Álvarez:** "Illustrated portrait of a young Argentine striker, boyish clean face, short neat hair, determined focused expression, energetic spider-like dynamism."
4. **Lautaro Martínez:** "Illustrated portrait of a strong Argentine striker, sharp jawline, short dark hair, fierce bull-like intensity, direct powerful gaze."
5. **De Paul:** "Illustrated portrait of an Argentine midfielder, long hair tied back, tattoos on neck, cocky bodyguard attitude, slight smirk, gritty raw texture."

Tip: generá los 5 en una misma sesión referenciando el primero para mantener coherencia de estilo, y pedí fondo transparente o recortable en círculo.

## Próximos pasos

1. Generar las 5 caras en Nano Banana con los prompts de arriba.
2. Pegar el brief en Claude Design (subiendo las 5 caras como referencia) y generar el mockup HTML completo.
3. Exportar el handoff (HTML + assets) y subirlo a esta conversación para integrarlo al plugin.
