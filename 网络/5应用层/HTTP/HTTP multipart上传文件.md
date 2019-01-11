# HTTP multipart/form-data上传文件

multipart/form-data的请求头必须包含一个特殊的头信息：Content-Type，且其值也必须规定为multipart/form-data，同时还需要规定一个内容分割符用于分割请求体中的多个post的内容，如文件内容和文本内容自然需要分割开来，不然接收方就无法正常解析和还原这个文件了。具体的头信息如下：Content-Type: multipart/form-data; boundary=${bound}
其中${bound} 是一个占位符，--${bound}代表我们规定的分割符，可以自己任意规定，但为了避免和正常文本重复了，尽量要使用复杂一点的内容，通常由客户端自主生成，如占位符为：yurnnlukjwfbrdiqvnqnegfitaaddkom，则分隔符为：--yurnnlukjwfbrdiqvnqnegfitaaddkom
