# bookgen
EPD opening book generator based on [Fairy-Stockfish](https://github.com/ianfab/Fairy-Stockfish).

Bookgen extends Fairy-Stockfish by the possibility of automatically generating opening books for all supported chess variants, including Chess, Xiangqi, Shogi, Crazyhouse, Antichess, Atomic, etc.

Several methods for generation and filtering of positions in FEN format are supported to adapt the book to specific requirements. The generation can be based on perft and multipv searches. The filtering supports a variety of options to define the criteria for evaluation of positions to be included. The output format is EPD. See the [Wiki](https://github.com/ianfab/bookgen/wiki) for documentation and usage examples.

Built binaries for Windows and Linux can be downloaded from the [releases](https://github.com/ianfab/bookgen/releases). Development versions can be downloaded from the [release workflow](https://github.com/ianfab/bookgen/actions/workflows/release.yml) (requires logging in to github). Instead of downloading it, you can also use the [online book generator](https://bookgen-wasm.vercel.app), which uses the [WebAssembly version](https://github.com/ianfab/fairy-stockfish.wasm/tree/bookgen).

There are separate (older) versions based on specialized variant forks, such as [official Stockfish](https://github.com/ianfab/bookgen/tree/official-stockfish), [multi-variant Stockfish](https://github.com/ianfab/bookgen/tree/multivariant), [Seirawan-Stockfish](https://github.com/ianfab/bookgen/tree/seirawan), [Shatranj-Stockfish](https://github.com/ianfab/bookgen/tree/shatranj), and [Makruk-Stockfish](https://github.com/ianfab/bookgen/tree/makruk) that can be found on the respective branches, but the Fairy-Stockfish based version supports all of these variants as well.

Also see the [Fairy-Stockfish](https://github.com/ianfab/Fairy-Stockfish) repository for more info.
