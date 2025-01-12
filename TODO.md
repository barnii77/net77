- [X] HTTP 1.0 support
- [X] basic TCP client/server implementation
- [X] functions for basic HTTP 1.0 requests (can communicate with actual websites)
- [X] more flexible TCP server implementation
- [X] ErrStatus and SuccessStatus typedefs (they're ints) with macros ecIsErr, ecIsOk, scIsOk, scIsErr to encode whether
  0 is no error and 1 is error or 1 is success and 0 no success...
- [ ] Make everything more stable (some of the tests don't always pass if you run them 1000 times)
    - [ ] testServerBigData1 seems to sometimes fail if I turn up the size of the messages enough AND run it very often
      in a loop with no pause in between
    - [X] the tests which make real requests to www.example.com seem to be unable to connect sometimes (but only if I
      run all the other tests as well....)
- [X] HTTP 1.1 support
- [X] Expand the unit testing helper into an entire little framework
- [X] Annotate nullable pointer parameters/struct fields with nullable macro
- [X] Annotate the rest of the pointers with the nonnull macro
- [X] If on clang, bake in _Nullable and _Nonnull when using nonnull and nullable
- [ ] Improve the kinda hacky HTTP 1.1 support by using the info from the HTTP 1.1 headers
    - [X] Should probably add optional callback to send-/recvAllData(Sb) which takes all data received up to date and
      returns how much more to receive
- [ ] Modify serialization and deserialization logic to handle Chunked Transfer Encoding etc there.
    - [ ] Potentially move this to a separate HTTP request preprocessing module to which any incoming raw http request
      should be passed first to normalize the request (i.e. remove CTE artifacts, move trailing CTE headers to the
      header section, ...)
- [ ] Use header fields like "Content-Length" in the isValidHttpRequest function
- [ ] Figure out why sendAllData takes so long: 500ms (or was it µs? anyway, its more than it should be) in
  testGetReqStreamResponse1->logging
- [ ] HTTP 2.0 support
- [ ] Some utilities for hosting HTTP/S servers (i.e. a tcp server handler wrapper)
- [ ] ALPN (application layer protocol negotiation)
- [ ] TLS 1.2 support (custom TLS implementation)
- [ ] TLS 1.3 support (custom TLS implementation)
- [ ] TLS session resumption
- [ ] HTTPS support
- [ ] proper error codes (not just 0, 1, -1 but actual error enums)
- [ ] HTTP 3.0 support
    - [ ] QUIC support
- [ ] Write and support zstd :)