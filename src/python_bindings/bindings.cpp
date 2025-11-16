#include <filesystem>
#include <initializer_list>

#include <pybind11/pybind11.h>

#include "ac/bind_ac.h"
#include "ar/bind_ar.h"
#include "bind_main_classes.h"
#include "cfd/bind_cfd.h"
#include "cfd/bind_cfd_verification.h"
#include "data/bind_data_types.h"
#include "dc/bind_dc_verification.h"
#include "dc/bind_fastadc.h"
#include "dd/bind_dd_verification.h"
#include "dd/bind_split.h"
#include "dynamic/bind_dynamic_fd_verification.h"
#include "fd/bind_fd.h"
#include "fd/bind_fd_verification.h"
#include "gfd/bind_gfd.h"
#include "gfd/bind_gfd_verification.h"
#include "ind/bind_ind.h"
#include "ind/bind_ind_verification.h"
#include "md/bind_md.h"
#include "md/bind_md_verification.h"
#include "mfd/bind_mfd_verification.h"
#include "nar/bind_nar.h"
#include "nd/bind_nd.h"
#include "nd/bind_nd_verification.h"
#include "od/bind_od.h"
#include "od/bind_od_verification.h"
#include "pfd/bind_pfd_verification.h"
#include "py_util/logging.h"
#include "sfd/bind_sfd.h"
#include "statistics/bind_statistics.h"
#include "ucc/bind_ucc.h"
#include "ucc/bind_ucc_verification.h"

namespace python_bindings {

PYBIND11_MODULE(desbordante, module, pybind11::mod_gil_not_used()) {
    using namespace pybind11::literals;
    for (auto bind_func : {BindMainClasses,
                           BindDataTypes,
                           BindLogging,
                           BindFd,
                           BindCfd,
                           BindAr,
                           BindUcc,
                           BindAc,
                           BindOd,
                           BindNd,
                           BindFdVerification,
                           BindMfdVerification,
                           BindNar,
                           BindUccVerification,
                           BindStatistics,
                           BindInd,
                           BindIndVerification,
                           BindGfdVerification,
                           BindSplit,
                           BindDynamicFdVerification,
                           BindNdVerification,
                           BindSFD,
                           BindMd,
                           BindMDVerification,
                           BindDCVerification,
                           BindPfdVerification,
                           BindFastADC,
                           BindGfd,
                           BindCFDVerification,
                           BindDDVerification,
                           BindAODVerification}) {
        bind_func(module);
    }
}

}  // namespace python_bindings
