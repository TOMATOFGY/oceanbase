/**
 * Copyright (c) 2021 OceanBase
 * OceanBase CE is licensed under Mulan PubL v2.
 * You can use this software according to the terms and conditions of the Mulan PubL v2.
 * You may obtain a copy of Mulan PubL v2 at:
 *          http://license.coscl.org.cn/MulanPubL-2.0
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PubL v2 for more details.
 */

#ifndef OCEABASE_STORAGE_OB_LS_META_
#define OCEABASE_STORAGE_OB_LS_META_

#include "lib/utility/utility.h"           // ObTimeGuard
#include "lib/lock/ob_spin_lock.h"
#include "logservice/palf/lsn.h"
#include "share/ob_ls_id.h"
#include "lib/ob_define.h"
#include "lib/function/ob_function.h"
#include "share/ob_unit_getter.h"
#include "storage/ls/ob_ls_state.h"
#include "storage/high_availability/ob_storage_ha_struct.h"
#include "logservice/ob_log_handler.h"
#include "logservice/ob_garbage_collector.h"
#include "share/restore/ob_ls_restore_status.h"
#include "storage/tx/ob_id_service.h"
#include "storage/ls/ob_ls_saved_info.h"

namespace oceanbase
{
namespace storage
{

class ObLSMeta
{
  OB_UNIS_VERSION_V(1);
public:
  ObLSMeta();
  ObLSMeta(const ObLSMeta &ls_meta);
  ~ObLSMeta() {}
  ObLSMeta &operator=(const ObLSMeta &other);
  int64_t get_clog_checkpoint_ts() const;
  palf::LSN &get_clog_base_lsn();
  int set_clog_checkpoint(const palf::LSN &clog_checkpoint_lsn,
                          const int64_t clog_checkpoint_ts,
                          const bool write_slog = true);
  void reset();
  bool is_valid() const;
  int64_t get_rebuild_seq() const;
  int set_migration_status(const ObMigrationStatus &migration_status,
                           const bool write_slog = true);
  int get_migration_status (ObMigrationStatus &migration_status) const;
  int set_gc_state(const logservice::LSGCState &gc_state);
  int get_gc_state(logservice::LSGCState &gc_state);
  int set_offline_ts_ns(const int64_t offline_ts_ns);
  int get_offline_ts_ns(int64_t &offline_ts_ns);
  int set_restore_status(const share::ObLSRestoreStatus &restore_status);
  int get_restore_status(share::ObLSRestoreStatus &restore_status) const;
  int update_ls_replayable_point(const int64_t replayable_point);
  int get_ls_replayable_point(int64_t &replayable_point);
  //for ha batch update ls meta element
  int update_ls_meta(
      const bool update_restore_status,
      const ObLSMeta &src_ls_meta);
  //for ha rebuild update ls meta
  int set_ls_rebuild();
  int check_valid_for_backup();
  int64_t get_tablet_change_checkpoint_ts() const;
  int set_tablet_change_checkpoint_ts(int64_t tablet_change_checkpoint_ts);
  int update_id_meta(const int64_t service_type,
                     const int64_t limited_id,
                     const int64_t latest_log_ts,
                     const bool write_slog);
  int update_id_service()
  {
    return all_id_meta_.update_id_service();
  }
  int get_saved_info(ObLSSavedInfo &saved_info);
  int build_saved_info();
  int set_saved_info(const ObLSSavedInfo &saved_info);
  int clear_saved_info();
  int get_migration_and_restore_status(
      ObMigrationStatus &migration_status,
      share::ObLSRestoreStatus &ls_restore_status);

  int init(
      const uint64_t tenant_id,
      const share::ObLSID &ls_id,
      const ObReplicaType &replica_type,
      const ObMigrationStatus &migration_status,
      const share::ObLSRestoreStatus &restore_status,
      const int64_t create_scn);
  class ObSpinLockTimeGuard
  {
  public:
    ObSpinLockTimeGuard(common::ObSpinLock &lock,
                        const int64_t warn_threshold = 100 * 1000 /* 100 ms */);
    ~ObSpinLockTimeGuard() {}
    void click(const char *mod = NULL) { time_guard_.click(mod); }
  private:
    ObTimeGuard time_guard_;
    ObSpinLockGuard lock_guard_;
  };
  TO_STRING_KV(K_(tenant_id), K_(ls_id), K_(replica_type), K_(ls_create_status),
               K_(clog_checkpoint_ts), K_(clog_base_lsn),
               K_(rebuild_seq), K_(migration_status), K(gc_state_), K(offline_ts_ns_),
               K_(restore_status), K_(replayable_point), K_(tablet_change_checkpoint_ts),
               K_(all_id_meta));
public:
  mutable common::ObSpinLock lock_;
  uint64_t tenant_id_;
  share::ObLSID ls_id_;
  ObReplicaType replica_type_;
  ObInnerLSStatus ls_create_status_;

private:
  typedef common::ObFunction<int(ObLSMeta &)> WriteSlog;
  // for test
  void set_write_slog_func_(WriteSlog write_slog);
  static WriteSlog write_slog_;

  // clog_checkpoint_ts_, meaning:
  // 1. dump points of all modules have exceeded clog_checkpoint_ts_
  // 2. all clog entries which log_ts are smaller than clog_checkpoint_ts_ can be recycled
  int64_t clog_checkpoint_ts_;
  // clog_base_lsn_, meaning:
  // 1. all clog entries which lsn are smaller than clog_base_lsn_ have been recycled
  // 2. log_ts of log entry that clog_base_lsn_ points to is smaller than/equal to clog_checkpoint_ts_
  // 3. clog starts to replay log entries from clog_base_lsn_ on crash recovery
  palf::LSN clog_base_lsn_;
  int64_t rebuild_seq_;
  ObMigrationStatus migration_status_;
  logservice::LSGCState gc_state_;
  int64_t offline_ts_ns_;
  share::ObLSRestoreStatus restore_status_;
  int64_t replayable_point_;
  int64_t tablet_change_checkpoint_ts_;
  transaction::ObAllIDMeta all_id_meta_;
  ObLSSavedInfo saved_info_;
};

}  // namespace storage
}  // namespace oceanbase
#endif
