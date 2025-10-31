# bookgen
Opening book generator based on [Fairy-Stockfish](https://github.com/fairy-stockfish/Fairy-Stockfish) with support for EPD and PGN output formats.

Bookgen extends Fairy-Stockfish by the possibility of automatically generating opening books for all supported chess variants, including Chess, Xiangqi, Shogi, Crazyhouse, Antichess, Atomic, etc.

Several methods for generation and filtering of positions are supported to adapt the book to specific requirements. The generation can be based on perft and multipv searches. The filtering supports a variety of options to define the criteria for evaluation of positions to be included. 

**Output formats supported:**
- **EPD format**: FEN positions (default, backward compatible)
- **PGN format**: Complete games with algebraic notation and headers

See the [Wiki](https://github.com/fairy-stockfish/bookgen/wiki) for documentation and usage examples.

Built binaries for Windows and Linux can be downloaded from the [releases](https://github.com/fairy-stockfish/bookgen/releases). Development versions can be downloaded from the [release workflow](https://github.com/fairy-stockfish/bookgen/actions/workflows/release.yml) (requires logging in to github).

Instead of downloading it, you can also use the [online book generator](https://bookgen-wasm.vercel.app) ([code](https://github.com/ianfab/bookgen-wasm)), which uses the [WebAssembly port](https://github.com/fairy-stockfish/fairy-stockfish.wasm/tree/bookgen).

## Usage Examples

### EPD Format (default, backward compatible)
```
setoption name BookFormat value epd
setoption name EPDPath value book.epd
position startpos
generate 3 depth 2
save
quit
```

### PGN Format (new functionality)
```
setoption name BookFormat value pgn
setoption name BookPath value book
position startpos
generate 3 depth 2
save
quit
```

The PGN format will create complete games with proper headers and algebraic notation, making it suitable for chess engines and analysis programs that prefer game-based input.

There are separate (older) versions based on specialized variant forks, such as [official Stockfish](https://github.com/fairy-stockfish/bookgen/tree/official-stockfish), [multi-variant Stockfish](https://github.com/fairy-stockfish/bookgen/tree/multivariant), [Seirawan-Stockfish](https://github.com/fairy-stockfish/bookgen/tree/seirawan), [Shatranj-Stockfish](https://github.com/fairy-stockfish/bookgen/tree/shatranj), and [Makruk-Stockfish](https://github.com/fairy-stockfish/bookgen/tree/makruk) that can be found on the respective branches, but the Fairy-Stockfish based version supports all of these variants as well.

Also see the [Fairy-Stockfish](https://github.com/fairy-stockfish/Fairy-Stockfish) repository for more info.
