Lbprate
=======

This tool fetches the current 1 USD to LBP buy and sell rate
from https://lbprate.com/,
and can fetch GTOG's buy rate
from https://www.omt.com.lb/en/services/payment/o-store

# Dependencies
- libcurl

# Building Dependencies
- gcc
- make

# Building
```
    git clone https://github.com/ahmadhadwan/Lbprate
    cd Lbprate
    make
```

# Usage
```
    ./bin/lbprate --help
```

# TODO
[X] Get the last rate update time.

[X] Add more usage options.
