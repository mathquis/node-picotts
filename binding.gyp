{
  "targets": [
    {
      "target_name": "picotts",
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "sources": [
        "./src/picotts.cpp",
        "./src/svoxpico/picoapi.c",
        "./src/svoxpico/picorsrc.c",
        "./src/svoxpico/picopal.c",
        "./src/svoxpico/picoknow.c",
        "./src/svoxpico/picoktab.c",
        "./src/svoxpico/picokpr.c",
        "./src/svoxpico/picoklex.c",
        "./src/svoxpico/picokdt.c",
        "./src/svoxpico/picokfst.c",
        "./src/svoxpico/picokpdf.c",
        "./src/svoxpico/picoctrl.c",
        "./src/svoxpico/picodata.c",
        "./src/svoxpico/picotok.c",
        "./src/svoxpico/picotrns.c",
        "./src/svoxpico/picopr.c",
        "./src/svoxpico/picowa.c",
        "./src/svoxpico/picosa.c",
        "./src/svoxpico/picoacph.c",
        "./src/svoxpico/picospho.c",
        "./src/svoxpico/picopam.c",
        "./src/svoxpico/picocep.c",
        "./src/svoxpico/picosig.c",
        "./src/svoxpico/picosig2.c",
        "./src/svoxpico/picobase.c",
        "./src/svoxpico/picofftsg.c",
        "./src/svoxpico/picoos.c"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")"
      ],
      "ldflags": [
      ],
      "cxxflags": [
        "-std=c++11",
        "-Wall",
        "-Winit-self"
      ],
      "cflags": [
        "-Wno-deprecated-declarations",
        "-Wno-sign-compare",
        "-Wno-unused-but-set-variable",
        "-Wno-unused-local-typedefs",
        "-Wno-unused-parameter",
        "-Wno-unused-variable",
        "-Wno-implicit-fallthrough"
      ],
      "library_dirs": [
      ],
      "libraries": [
      ],
      'defines': [ 'NAPI_DISABLE_CPP_EXCEPTIONS' ],
    }
  ]
}
