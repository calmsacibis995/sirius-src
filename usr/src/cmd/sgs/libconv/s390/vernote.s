	.section	".note"

#include <sgs.h>

	.align	4
	.word	.endname - .startname	/* note name size */
	.word	0			/* note desc size */
	.word	0			/* note type */
.startname:
	.ascii	"\tSolaris Link Editors: 5.11-\n"
.endname:

	.section	".rodata", #alloc
	.global		link_ver_string
link_ver_string:
	.type		link_ver_string, #object
	.ascii	"5.11-1.1615\0"
	.size	link_ver_string, .-link_ver_string
