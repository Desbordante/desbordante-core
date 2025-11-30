#include <filesystem>
#include <initializer_list>

#include <pybind11/pybind11.h>

#include "python_bindings/ac/bind_ac.h"
#include "python_bindings/afd_metric/bind_afd_metric_calculation.h"
#include "python_bindings/ar/bind_ar.h"
#include "python_bindings/bind_main_classes.h"
#include "python_bindings/cfd/bind_cfd.h"
#include "python_bindings/cfd/bind_cfd_verification.h"
#include "python_bindings/data/bind_data_types.h"
#include "python_bindings/dc/bind_dc_verification.h"
#include "python_bindings/dc/bind_fastadc.h"
#include "python_bindings/dd/bind_dd_verification.h"
#include "python_bindings/dd/bind_split.h"
#include "python_bindings/dynamic/bind_dynamic_fd_verification.h"
#include "python_bindings/fd/bind_fd.h"
#include "python_bindings/fd/bind_fd_verification.h"
#include "python_bindings/gfd/bind_gfd.h"
#include "python_bindings/gfd/bind_gfd_verification.h"
#include "python_bindings/ind/bind_ind.h"
#include "python_bindings/ind/bind_ind_verification.h"
#include "python_bindings/md/bind_md.h"
#include "python_bindings/md/bind_md_verification.h"
#include "python_bindings/mfd/bind_mfd_verification.h"
#include "python_bindings/nar/bind_nar.h"
#include "python_bindings/nd/bind_nd.h"
#include "python_bindings/nd/bind_nd_verification.h"
#include "python_bindings/od/bind_od.h"
#include "python_bindings/od/bind_od_verification.h"
#include "python_bindings/pfd/bind_pfd_verification.h"
#include "python_bindings/py_util/logging.h"
#include "python_bindings/sfd/bind_sfd.h"
#include "python_bindings/statistics/bind_statistics.h"
#include "python_bindings/ucc/bind_ucc.h"
#include "python_bindings/ucc/bind_ucc_verification.h"


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
