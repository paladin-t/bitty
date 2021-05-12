## Asset Formats

Bitty Engine supports variant audio, image and font types that are widely used. Furthermore it supports sprite, map, palette and paletted image defined as JSON. This document examples these JSON-based asset formats.

### Sprite

```js
{
  "width": 8, "height": 8,
  "count": 5,                  /* Frame count. */
  "data": [
    {
      "x": 0, "y": 0,
      "width": 8, "height": 8,
      "interval": 0.25,
      "key": "idle"            /* Named frame. */
    },
    {
      "x": 0, "y": 0,
      "width": 8, "height": 8,
      "interval": 0.25,
      "key": ""
    },
    ...                        /* More... */
  ],
  "ref": "bank.png"            /* Ref to an image asset. */
}
```

### Map

```js
{
  "tiles": {
    "count": [
      8, 8                    /* How many tiles in x, y axises respectively of the tiles ref. */
    ]
  },
  "width": 120, "height": 80,
  "data": [
    0, 0, 1, 2,               /* Cel indices. */
    ...                       /* More... */
  ],
  "ref": "bank.png"           /* Ref to an image asset as tiles. */
}
```

### Palette

```js
{
  "count": 256,     /* Up to 256 colors. */
  "data": [
    [ 0, 0, 0, 0 ], /* [R, G, B, A]. */
    ...             /* More... */
  ]
}
```

### (Paletted) Image

```js
{
  "width": 128, "height": 128,
  "depth": 8,                  /* Up to 256 colors. */
  "data": [
    0, 0, 1, 2,                /* Color indices. */
    ...                        /* More... */
  ],
  "ref": "palette.pal"         /* Ref to a palette asset. */
}
```
