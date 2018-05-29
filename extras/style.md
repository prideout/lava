## Internal code style

The code is vertically compact, but no single line should be longer than 100 characters. All
public-facing Lava types live in the `par` namespace.

For `#include`, always use angle brackets unless including a private header that lives in the same
directory. Includes are arranged in blocks, where each block is an alphabetized list of headers. The
first block is composed of `par` headers, followed by a sorted list of blocks for each `extern/`
library, followed by a block of C++ STL headers, followed by the block of standard C headers,
followed by the block of private headers. For example:

```cpp
#include <par/LavaContext.h>
#include <par/LavaLog.h>

#include <SPIRV/GlslangToSpv.h>

#include <string>
#include <vector>

#include "LavaInternal.h"
```

Methods and functions should have comments that are descriptive ("Opens the file") rather than
imperative ("Open the file").
