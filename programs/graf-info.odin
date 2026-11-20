package graf_info

import "core:fmt"
import "core:os"
import "core:strings"
import "graf:graf"

main :: proc() {
  G: graf.Graf
  graf.init(&G)
  defer graf.close(&G)

  if len(os.args) < 2 {
    fmt.eprintln("Usage: graf-info [FILE]")
    os.exit(1)
  }

  path_c := strings.clone_to_cstring(os.args[1])
  status := graf.open(path_c, &G)
  defer delete(path_c)

  if status != .OK {
    fmt.eprintln("graf_open() failed", graf.error_info(&G, status))
    graf.close(&G)
    os.exit(2)
  }

  V := make([]graf.Vertex, G.nv > 0 ? G.nv : 1)
  A := make([]graf.Arc, G.na > 0 ? G.na : 1)
  s := make([]u8, G.nchars > 0 ? G.nchars : 1)

  status = graf.load(&G, raw_data(V), raw_data(A), cast(cstring)raw_data(s))

  if status != .OK {
    fmt.eprintln("graf_load() failed", graf.error_info(&G, status))
    graf.close(&G)
    os.exit(2)
  }

  fmt.eprintln("no errors")

  delete(V)
  delete(A)
  delete(s)
}
