#!/bin/bash
#./rijndael_slice input_large.asc output_large.enc e 1234567890abcdeffedcba09876543211234567890abcdeffedcba0987654321

key=(
"5dad7973f3ab6aec4bba4be8d90ff6ba0fb588ca285392c2979204818d153565"
"fa89dbb5e100cc4e7070d15e1fcfbdf86d90bae616734f5d3f1973bfcfd2bb15"
"8b1ed13fcd7ccb5a56a6f018441fe832bb022620f0ada03e01b7401850b2fbc6"
"22fd2b5fb66a3acbf15a84c83c8269a0cdd399004a53ca99f5d41ffc28d00aee"
"a0761538e968d7cdd833d2c6ef407782193362dfc69efd8e3381f08d661aece7"
"c8f9ee6be20a756213da85da8ee7f1c35e5a84473e080bbf74baf8d0b4d7cfee"
"537ef6ec3216b58e64e78b11558d2165d7b6ad4de61cd668742a227ac2621ce2"
"0db5c9ef11e994f277468d9c74ac9b8be374b1c4c4f03b05639c3c5d361f1283"
"fda38dd0d320c9a3798b8f28e6c1b490ef7decc8ba7547c5566823bd8c545fd4"
"f5308b603b67c427b152326d6a836153433787a6474dd12a0b1b4a61f1fe9911"
"f8eb5c7836eef2fae6412f58c241d0e9552a0ca42cb30841c2029c6aa4b33942"
"635f3a8b0a1a1b8059cb4cb6468d0a2620cc8ade072aa923f38c4d1d99764509"
"4e1a7c2e5faf0d05d65d7a4dd08f66cb5f75cb6ac9e108f24e065cd7bb37a3e8"
"8cc2ac6295f67b632418e30de60f434b515c5010cd99cd99b67b128083f613f2"
"d6b3627b8f11a8349ec3d3a40230b58749da9edb33a926ab8a9a377ff7cd500b"
"dddf7157c0fab7e561472e16931f77a05987a2b587879bc622a13ba7d6b804ab"
"52b6e99b827c0ed554060d3fb76fce7aeb11555ca4782724f2bdfe0069ad8f86"
"360f789b6e637bf63fff343f5ebbb71415d241f4312c5a447284b0fcd06d27b8"
"9a7a7efa9511bb222a83969c42513f4b6042e78f086be005ec2d250eedca7405"
"ca0586220a91c8b0db3d04caa97e019be89a916a72e32d6710f56794db8015c5"
"b7e95c192961105ce81302a7c9aca793ebafa4f6cad51deba19ad85fb777af71"
"7a58c519572318af93b978e32bbd352d70d844d5d03f554005f6753e5f83db6c"
"500c75c9cc7528fc46cccfde3cbfb928eecd723ad761f81bd49f65cd48be45e5"
"16f662dd354f638cae1cb15c43d23f91e32245b723f5868a261f73d7b8ba0be7"
"fd9c871b1b2dcd8601b860dd7ab1b168e6e6f43bb4050199f0267be5caa68d78"
"3eae56da69b22a36b0c787ac0b67ee3c18fb6eaa8847541c18b0a213c8fe10db"
"b937805b09dacf1e45d36cad83e3622adc6c9a071b3a09d1e7eae4b7cfe96c84"
"c2e107f6f5619d218ad2bf703394ddcab49b01f305e83408884e85781261944a"
"8b494ebb5f30501dcd267580a045482f7e25d3c9bce6859ec820289b53e1db84"
"538688564b2abbab7f9fc963b616bf8127c4b3961108dd700dd33f4818a8b7e4"
"4bb2d8f2447499ed4108b228ddf9522bccad48cc15d1aa23b0f758c5a53d2c50"
"2eeaace19f802c5f29a8a9bf572ca73eba6fb2649a507b766ef98dca69ff9353"
"0c6a36273387611986c1a0807baf3e64dec494698e436b4d7a2391e24fff19b4"
"3d5bd5c503280b3d16cd8fcf67f760c1ec21990440a2aa50b5cdb749d220488c"
"e9fa6a3e235a07f62671664d32f142c2e76de89e09daec9f32d652a803f8f782"
"47cc2e58fa98d974b63752af2c7af6d7b787f901c3bf44663b63ad79a5e85fab"
"4ab2c0e1a5ccdf05dbf7cbb4014aa370dd6467cd2c38cd34f3b1b3739a60125f"
"0458f965e93e0e45f21b605519a3f6bc01250950d65cc4cc6eaffca797f5ffbe"
"9d85614e679ba92f4e1856395a5926a4936c80b990e314dcabfb20fd00fd9823"
"12a711e97f339408df83bfe5d6a526cdce0c63608c2491d08b41482563b59096"
"dcbbefc69e7151bc8493a76397ba7b7e96bd018c5f7df9b56c6d8ea976edbdaa"
"ccf8ec4216438fc2a87582808f58e633e8dbceb42a0312fefe552e478a90d272"
"5702f60285fe2816eeca988c221c46f283fc2ac012be35fcf2d9e07711058686"
"9b6f12fd0a9568ced5e60683a488d00a1949393467f631d0aa00529e5b4ea0c8"
"d71947452fd694ed172850e4b3d9984688c1a1eb8798da0bd8f1a063362dad06"
"1423717cc1d96c1ae23ae2668b952c80a58ef49315ee15707ebcaf8e609ccdd6"
"f42f761e36182eae463fae8c8687f965d2b721cac5088f4c8420a2b2f271fe7e"
"1f8fb5bf04666c7a25056a6c2a59825e9142a0f801f4fb9ec36c288b0669d7e0"
"9f8403a11342d34f18f8e3d84c3351816483f6c92a5ee9763410bfc4cb021f60"
"321cab4335658cfc8d819beec8d17501168ae0ecdc78ab49de96d0ad30913e54"
"ade0f3a6448963135e9dadc61031fe5dcc36ce46ede8268d85406d06c428cd92"
"73ac5df66adb458bd4d125f0e016880ae8d8eae9fe04b9c822ff3db31421ee27"
"740b4d5429f365ee7454cd871bfe83b1be91ceeb097bedb92502fdf561314e18"
"f860f57e5b924d8f08375a1711d3f450b09cf5c11130f559cb1f7cb6d9af2df2"
"b4c2466384456c3d5ded34ffb50d2ac8b70f439de20b02ae8fed6db8f3665ed3"
"28b52e6293da3739d482b0f1dc6663c70d482fd209fad1f61ed6a172bc7ec466"
"c8b9ffc9a6180acd5379af8cc2287f1ef8a0aa83ab7be75ee20c3ef14133ccff"
"6f4865c5df663ae340c4c12feccf7b5bce459502397ba07dea1a2f2c37e466b1"
"544f182860f171a1c37b2009a20e1ba825e9ee464421cdc13f870d9868738e78"
"dae7e39c1b0ada11ca43a4bdf429dfb3b51bc5003c0ab59649bd145a9ab23b90"
"9aa2a2f8cc63c1df0908ba2f868d16b2ea25f71759ce7bc968b8c305b1785004"
"b43fb38dcb3b03d8386badeed52b5f006f1f66b13e5cff2d990a418bd92e7d8b"
"060a33bbf674a9cb630e1d6987771a4441467f42d7cbb6621b545ffbcbbfac97"
"8ad377e7962a6861f3e52716c983bf19c40956604324d5cba3399de26901fe7c"
"5ccaf6e66a0b2b00baaff2a60ec9267f9ac7fc70d4a1669332d3ef9186753ebf"
"9f4b0bfde7555e43d97ea0e01d33535122cd3f6aff42cb3d210d6dc32cd5c20b"
"256ed1b752116fa62f0b166cad1ba40d40a47e0041da77ffaa65e48bd8611fa4"
"037e2fb8e0ee2bfbec553f9346cc55db613ef3481ceae10d1c5d833882c60a99"
"80868c4a5dff0a301d281939d3fd6b157e11532fda3ea082caca742a3f64f88a"
"7510bdbc4c0613c41c3583bedabde838e4fa5d92ef35480f655eee75032221cc"
"a11e7fa63e8d5b4774bf7e4c4412d26069be85a0844a72f7a4c16cba25c3d6ed"
"6142a929a6602e12c2fa9f389c3a344c1537684576ab1c3298be87862327b094"
"5f697ca310289474bcb4f23efcf7941ac157dd4ac7445f158554eea2eb58a593"
"3b6729f52168462bd32520869d14ac90a03c865e4a07746ba52bfc8315ead265"
"140146ff6c630c0e4d589bec92af5ceb99811e4335a5d2052c4caf55f1c94a6d"
"e0ae33c7c041169476b749780607870caa3bb91696ce2e1e81e5568390b1d223"
"3aad6c397bc97ba06b34f010ad7aeb93199ec2b9e404af99c8393866393a3769"
"b605fb3f88dc8f2723edd6f098b2217a0f1d95acfa2bf4b58c3919841823e7fd"
"2012e3378962dfa7d7c7c6a9ba8eca2e8d34bd5db3f65586da9491619b784072"
"6b763c4e6997be18dd3e80060d4cc660fe964bc30fcdccaef7a59cb8f42445b6"
"d8e3a59199eb080e2d20a250e1c98dff7a6324e5248d8be98e6ef999183886c4"
"f4aa933e35d3d2b3ddb3263b9b351874df6d21a0607d5e148ea284ed948f6272"
"6dfc068ee41f5f5b06e7ffc7c4e4aa1fb6696a40d6af5c533264ce3e89e34f7b"
"f1c4019456230cebed8d6a97f113fbfdf493bcd2e4b38ed502f546bc29416f48"
"d52f6093fdb244988735d59a59ec32519892e74bfc663645e307ff542e04571f"
"995a511f682a974a053bf7dc6a7f6ca30c0765418e670c4e1c8a9bf2a55bd1cc"
"140d71cc5409c2ba003f38068344859826077a5bac429c4d631668b2d524ba30"
"7c9415705990a3a12e08c7c0318532f6056791bff531966f2213b780d4aedab3"
"cf5108b3b51f1357817d53e4730078836987ded372d945d18e7f3549b290838e"
"da0e37efff06fc187580c5dced4518794aea9aa1dc5079dc5e7dff9a32c169f8"
"295b4ef96c5fa65e842bd876d2e90c3af6b46df6b3e6df25269bb363efd956ae"
"06c512df8687256427487c14a3faa3690d986a1b60991c7c31872ea7ad9f42c8"
"c75f01028d2ce8c0ef6701f0e8de69af05b66274ee96b95b426634793340640a"
"48fffc5538eacdba45b919cdbab5b1eab7ca41c2aea74503915a5a0239892339"
"47de33d8dc75987800bd8bd22863f182865628b04300ecf17cafaf114498540c"
"7628e5d0dc53d4c8c1e07de188c94b24c77ac1e649b96410cfbc9e1172077f7c"
"b50dbc5fd50f2d4146fef89c68b91a2507c1f97a2177df52c7f20a7e7675f0f9"
"a5e58c7d15971bddf3c06b8b77998290ec747076ff48751105ed842a500d2b8d"
"168afaefe4e9893a9410af4175fa94f480b0371dfd8b1b86d4c87557c1d351d4"
"01868e9468b3962233badf4364b74c1278c2cc385310af0d88cf5ccbf95d0539"
)

for i in {1..100}
do
  #key=$(cat /dev/urandom | LC_CTYPE=C tr -dc 'a-f0-9' | fold -w 64 | head -n 1)
  #echo \"$key\" 
  #./input_generator.py > input_slice.asc
  ./rijndael input_fixed.asc output_large.enc e ${key[i]}
done

#./rijndael_slice output_large.enc output_large.dec d 1234567890abcdeffedcba09876543211234567890abcdeffedcba0987654321

