# bookgen
EPD opening book generator based on [Fairy-Stockfish](https://github.com/fairy-stockfish/Fairy-Stockfish).

Bookgen extends Fairy-Stockfish by the possibility of automatically generating opening books for all supported chess variants, including Chess, Xiangqi, Shogi, Crazyhouse, Antichess, Atomic, etc.

Several methods for generation and filtering of positions in FEN format are supported to adapt the book to specific requirements. The generation can be based on perft and multipv searches. The filtering supports a variety of options to define the criteria for evaluation of positions to be included. The output format is EPD. See the [Wiki](https://github.com/fairy-stockfish/bookgen/wiki) for documentation and usage examples.

Built binaries for Windows and Linux can be downloaded from the [releases](https://github.com/fairy-stockfish/bookgen/releases). Development versions can be downloaded from the [release workflow](https://github.com/fairy-stockfish/bookgen/actions/workflows/release.yml) (requires logging in to github).

Instead of downloading it, you can also use the [online book generator](https://bookgen-wasm.vercel.app) ([code](https://github.com/ianfab/bookgen-wasm)), which uses the [WebAssembly port](https://github.com/fairy-stockfish/fairy-stockfish.wasm/tree/bookgen).
