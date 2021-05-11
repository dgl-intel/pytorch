from typing import List, Union, Optional

from tools.codegen.context import with_native_function_and_index
from tools.codegen.utils import mapMaybe
from tools.codegen.model import NativeFunction, NativeFunctionsGroup, BackendIndex
from tools.codegen.api.types import kernel_signature
import tools.codegen.api.meta as meta
import tools.codegen.api.structured as structured

@with_native_function_and_index
def gen_unstructured(f: NativeFunction, backend_index: BackendIndex) -> Optional[str]:
    sig = kernel_signature(f, backend_index)
    metadata = backend_index.get(f)
    if metadata is None:
        return None
    if "legacy::" in metadata.kernel:
        return None
    else:
        prefix = 'static' if backend_index.external else 'TORCH_API'
        return f"{prefix} {sig.decl(name=metadata.kernel)};"

@with_native_function_and_index
def gen_structured(g: NativeFunctionsGroup, backend_index: BackendIndex) -> List[str]:
    meta_name = meta.name(g)
    out_args = structured.impl_arguments(g)
    metadata = backend_index.get(g)
    if metadata is None:
        return []
    prefix = 'static' if backend_index.external else 'TORCH_API'
    return [f"""\
struct {prefix} structured_{metadata.kernel} : public at::meta::{meta_name} {{
void impl({', '.join(a.decl() for a in out_args)});
}};
"""]

# Generates NativeFunctions.h, a list of forward declarations of all
# actual kernel definitions we keep in aten/src/ATen/native/
@with_native_function_and_index
def compute_native_function_declaration(
        g: Union[NativeFunctionsGroup, NativeFunction],
        backend_index: BackendIndex
) -> List[str]:
    metadata = backend_index.get(g)
    if isinstance(g, NativeFunctionsGroup):
        if metadata is not None and metadata.structured:
            if backend_index.external:
                # Structured hasn't been tested with external backends yet.
                raise AssertionError("Structured external backend functions are not implemented yet.")
            else:
                return gen_structured(g, backend_index)
        else:
            return list(mapMaybe(lambda f: gen_unstructured(f, backend_index), g.functions()))
    else:
        x = gen_unstructured(g, backend_index)
        return [] if x is None else [x]
