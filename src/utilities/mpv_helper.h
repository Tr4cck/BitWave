//
// Created by Reverier-Xu on 2021/6/27.
//

#pragma once

#include <cstring>

namespace mpv::qt {

// Wrapper around mpv_handle. Does ref counting under the hood.
    class Handle {
        struct container {
            explicit container(mpv_handle *h) : mpv(h) {}

            ~container() { mpv_terminate_destroy(mpv); }

            mpv_handle *mpv{};
        };

        QSharedPointer<container> qSharedPointer{};

    public:
        // Construct a new Handle from a raw mpv_handle with refcount 1. If the
        // last Handle goes out of scope, the mpv_handle will be destroyed with
        // mpv_terminate_destroy().
        // Never destroy the mpv_handle manually when using this wrapper. You
        // will create dangling pointers. Just let the wrapper take care of
        // destroying the mpv_handle.
        // Never create multiple wrappers from the same raw mpv_handle; copy the
        // wrapper instead (that's what it's for).
        static Handle FromRawHandle(mpv_handle *handle) {
            Handle h;
            h.qSharedPointer = QSharedPointer<container>(new container(handle));
            return h;
        }

        // Return the raw handle; for use with the libmpv C API.
        explicit operator mpv_handle *() const {
            return qSharedPointer ? (*qSharedPointer).mpv : nullptr;
        }
    };

    static inline QVariant node_to_variant(const mpv_node *node) {
        switch (node->format) {
            case MPV_FORMAT_STRING:
                return QVariant(QString::fromUtf8(node->u.string));
            case MPV_FORMAT_FLAG:
                return QVariant(static_cast<bool>(node->u.flag));
            case MPV_FORMAT_INT64:
                return QVariant(static_cast<qlonglong>(node->u.int64));
            case MPV_FORMAT_DOUBLE:
                return QVariant(node->u.double_);
            case MPV_FORMAT_NODE_ARRAY: {
                mpv_node_list *list = node->u.list;
                QVariantList qlist;
                for (int n = 0; n < list->num; n++)
                    qlist.append(node_to_variant(&list->values[n]));
                return QVariant(qlist);
            }
            case MPV_FORMAT_NODE_MAP: {
                mpv_node_list *list = node->u.list;
                QVariantMap qmap;
                for (int n = 0; n < list->num; n++) {
                    qmap.insert(QString::fromUtf8(list->keys[n]),
                                node_to_variant(&list->values[n]));
                }
                return QVariant(qmap);
            }
            default:  // MPV_FORMAT_NONE, unknown values (e.g. future extensions)
                return QVariant();
        }
    }

    struct node_builder {
        explicit node_builder(const QVariant &v) { set(&node_, v); }

        ~node_builder() { free_node(&node_); }

        mpv_node *node() { return &node_; }

    private:
        Q_DISABLE_COPY(node_builder)

        mpv_node node_{};

        mpv_node_list *create_list(mpv_node *dst, bool is_map, int num) {
            dst->format = is_map ? MPV_FORMAT_NODE_MAP : MPV_FORMAT_NODE_ARRAY;
            auto *list = new mpv_node_list();
            dst->u.list = list;
            list->values = new mpv_node[num]();
            if (!list->values) goto err;
            if (is_map) {
                list->keys = new char *[num]();
                if (!list->keys) goto err;
            }
            return list;
            err:
            free_node(dst);
            return nullptr;
        }

        static char *dup_qstring(const QString &s) {
            QByteArray b = s.toUtf8();
            char *r = new char[b.size() + 1];
            std::memcpy(r, b.data(), b.size() + 1);
            return r;
        }

        static bool test_type(const QVariant &v, QMetaType::Type t) {
            // The Qt docs say: "Although this function is declared as returning
            // "QVariant::Type(obsolete), the return value should be interpreted
            // as QMetaType::Type."
            // So a cast really seems to be needed to avoid warnings (urgh).
            return static_cast<int>(v.type()) == static_cast<int>(t);
        }

        void set(mpv_node *dst, const QVariant &src) {
            if (test_type(src, QMetaType::QString)) {
                dst->format = MPV_FORMAT_STRING;
                dst->u.string = dup_qstring(src.toString());
                if (!dst->u.string) goto fail;
            } else if (test_type(src, QMetaType::Bool)) {
                dst->format = MPV_FORMAT_FLAG;
                dst->u.flag = src.toBool() ? 1 : 0;
            } else if (test_type(src, QMetaType::Int) ||
                       test_type(src, QMetaType::LongLong) ||
                       test_type(src, QMetaType::UInt) ||
                       test_type(src, QMetaType::ULongLong)) {
                dst->format = MPV_FORMAT_INT64;
                dst->u.int64 = src.toLongLong();
            } else if (test_type(src, QMetaType::Double)) {
                dst->format = MPV_FORMAT_DOUBLE;
                dst->u.double_ = src.toDouble();
            } else if (src.canConvert<QVariantList>()) {
                QVariantList qList = src.toList();
                mpv_node_list *list = create_list(dst, false, qList.size());
                if (!list) goto fail;
                list->num = qList.size();
                for (int n = 0; n < qList.size(); n++)
                    set(&list->values[n], qList[n]);
            } else if (src.canConvert<QVariantMap>()) {
                QVariantMap qMap = src.toMap();
                mpv_node_list *list = create_list(dst, true, qMap.size());
                if (!list) goto fail;
                list->num = qMap.size();
                for (int n = 0; n < qMap.size(); n++) {
                    list->keys[n] = dup_qstring(qMap.keys().at(n));
                    if (!list->keys[n]) {
                        free_node(dst);
                        goto fail;
                    }
                    set(&list->values[n], qMap.values().at(n));
                }
            } else {
                goto fail;
            }
            return;
            fail:
            dst->format = MPV_FORMAT_NONE;
        }

        void free_node(mpv_node *dst) {
            switch (dst->format) {
                case MPV_FORMAT_STRING:
                    delete[] dst->u.string;
                    break;
                case MPV_FORMAT_NODE_ARRAY:
                case MPV_FORMAT_NODE_MAP: {
                    mpv_node_list *list = dst->u.list;
                    if (list) {
                        for (int n = 0; n < list->num; n++) {
                            if (list->keys) delete[] list->keys[n];
                            if (list->values) free_node(&list->values[n]);
                        }
                        delete[] list->keys;
                        delete[] list->values;
                    }
                    delete list;
                    break;
                }
                default:;
            }
            dst->format = MPV_FORMAT_NONE;
        }
    };

/**
 * RAII wrapper that calls mpv_free_node_contents() on the pointer.
 */
    struct node_auto_free {
        mpv_node *ptr{};

        explicit node_auto_free(mpv_node *a_ptr) : ptr(a_ptr) {}

        ~node_auto_free() { mpv_free_node_contents(ptr); }
    };

/**
 * Return the given property as mpv_node converted to QVariant, or QVariant()
 * on error.
 *
 * @deprecated use get_property() instead
 *
 * @param name the property name
 */
    static inline QVariant get_property_variant(mpv_handle *ctx,
                                                const QString &name) {
        mpv_node node;
        if (mpv_get_property(ctx, name.toUtf8().data(), MPV_FORMAT_NODE, &node) < 0)
            return QVariant();
        node_auto_free f(&node);
        return node_to_variant(&node);
    }

/**
 * Set the given property as mpv_node converted from the QVariant argument.
 * @deprecated use set_property() instead
 */
    static inline int set_property_variant(mpv_handle *ctx, const QString &name,
                                           const QVariant &v) {
        node_builder node(v);
        return mpv_set_property(ctx, name.toUtf8().data(), MPV_FORMAT_NODE,
                                node.node());
    }

/**
 * Set the given option as mpv_node converted from the QVariant argument.
 *
 * @deprecated use set_property() instead
 */
    static inline int set_option_variant(mpv_handle *ctx, const QString &name,
                                         const QVariant &v) {
        node_builder node(v);
        return mpv_set_option(ctx, name.toUtf8().data(), MPV_FORMAT_NODE,
                              node.node());
    }

/**
 * mpv_command_node() equivalent. Returns QVariant() on error (and
 * unfortunately, the same on success).
 *
 * @deprecated use command() instead
 */
    static inline QVariant command_variant(mpv_handle *ctx, const QVariant &args) {
        node_builder node(args);
        mpv_node res;
        if (mpv_command_node(ctx, node.node(), &res) < 0) return QVariant();
        node_auto_free f(&res);
        return node_to_variant(&res);
    }

/**
 * This is used to return error codes wrapped in QVariant for functions which
 * return QVariant.
 *
 * You can use get_error() or is_error() to extract the error status from a
 * QVariant value.
 */
    struct ErrorReturn {
        /**
         * enum mpv_error value (or a value outside of it if ABI was extended)
         */
        int error;

        ErrorReturn() : error(0) {}

        explicit ErrorReturn(int err) : error(err) {}
    };

/**
 * Return the mpv error code packed into a QVariant, or 0 (success) if it's not
 * an error value.
 *
 * @return error code (<0) or success (>=0)
 */
    static inline int get_error(const QVariant &v) {
        if (!v.canConvert<ErrorReturn>()) return 0;
        return v.value<ErrorReturn>().error;
    }

/**
 * Return whether the QVariant carries a mpv error code.
 */
    static inline bool is_error(const QVariant &v) { return get_error(v) < 0; }

/**
 * Return the given property as mpv_node converted to QVariant, or QVariant()
 * on error.
 *
 * @param name the property name
 * @return the property value, or an ErrorReturn with the error code
 */
    static inline QVariant get_property(mpv_handle *ctx, const QString &name) {
        mpv_node node;
        int err =
                mpv_get_property(ctx, name.toUtf8().data(), MPV_FORMAT_NODE, &node);
        if (err < 0) return QVariant::fromValue(ErrorReturn(err));
        node_auto_free f(&node);
        return node_to_variant(&node);
    }

/**
 * Set the given property as mpv_node converted from the QVariant argument.
 *
 * @return mpv error code (<0 on error, >= 0 on success)
 */
    static inline int set_property(mpv_handle *ctx, const QString &name,
                                   const QVariant &v) {
        node_builder node(v);
        return mpv_set_property(ctx, name.toUtf8().data(), MPV_FORMAT_NODE,
                                node.node());
    }

/**
 * mpv_command_node() equivalent.
 *
 * @param args command arguments, with args[0] being the command name as string
 * @return the property value, or an ErrorReturn with the error code
 */
    static inline QVariant command(mpv_handle *ctx, const QVariant &args) {
        node_builder node(args);
        mpv_node res;
        int err = mpv_command_node(ctx, node.node(), &res);
        if (err < 0) return QVariant::fromValue(ErrorReturn(err));
        node_auto_free f(&res);
        return node_to_variant(&res);
    }

}  // namespace mpv::qt

Q_DECLARE_METATYPE(mpv::qt::ErrorReturn)
