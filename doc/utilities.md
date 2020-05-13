# Utility functions {#api_utility}

Some additional functions are used internally and also exposed in the
API:

- platform independent large file utils: rs_file_open(), rs_file_close(),
  rs_file_size(), rs_file_copy_cb().

- encoding/decoding binary data: rs_base64(), rs_unbase64(),
  rs_hexify().

- MD4 message digests: rs_mdfour(), rs_mdfour_begin(),
  rs_mdfour_update(), rs_mdfour_result().
