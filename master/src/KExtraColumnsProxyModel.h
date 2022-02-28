/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
    SPDX-FileContributor: David Faure <david.faure@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KEXTRACOLUMNSPROXYMODEL_H
#define KEXTRACOLUMNSPROXYMODEL_H

#include <QIdentityProxyModel>
#include <QScopedPointer>

class KExtraColumnsProxyModelPrivate;

/**
 * @class KExtraColumnsProxyModel KExtraColumnsProxyModel.h KExtraColumnsProxyModel
 *
 * This proxy appends extra columns (after all existing columns).
 *
 * The proxy supports source models that have a tree structure.
 * It also supports editing, and propagating changes from the source model.
 * Row insertion/removal, column insertion/removal in the source model are supported.
 *
 * Not supported: adding/removing extra columns at runtime; having a different number of columns in subtrees;
 * drag-n-drop support in the extra columns; moving columns.
 *
 * Derive from KExtraColumnsProxyModel, call appendColumn (typically in the constructor) for each extra column,
 * and reimplement extraColumnData() to allow KExtraColumnsProxyModel to retrieve the data to show in the extra columns.
 *
 * If you want your new column(s) to be somewhere else than at the right of the existing columns, you can
 * use a KRearrangeColumnsProxyModel on top.
 *
 * Author: David Faure, KDAB
 * @since 5.13
 */
class KExtraColumnsProxyModel : public QIdentityProxyModel
{
    Q_OBJECT
public:
    /**
     * Base class constructor.
     * Remember to call setSourceModel afterwards, and appendColumn.
     */
    explicit KExtraColumnsProxyModel(QObject *parent = nullptr);
    /**
     * Destructor.
     */
    ~KExtraColumnsProxyModel() override;

    // API

    /**
     * Appends an extra column.
     * @param header an optional text for the horizontal header
     * This does not emit any signals - do it in the initial setup phase
     */
    void appendColumn(const QString &header = QString());

    /**
     * Removes an extra column.
     * @param idx index of the extra column (starting from 0).
     * This does not emit any signals - do it in the initial setup phase
     * @since 5.24
     */
    void removeExtraColumn(int idx);

    /**
     * This method is called by data() for extra columns.
     * Reimplement this method to return the data for the extra columns.
     *
     * @param parent the parent model index in the proxy model (only useful in tree models)
     * @param row the row number for which the proxy model is querying for data (child of @p parent, if set)
     * @param extraColumn the number of the extra column, starting at 0 (this doesn't require knowing how many columns the source model has)
     * @param role the role being queried
     * @return the data at @p row and @p extraColumn
     */
    virtual QVariant extraColumnData(const QModelIndex &parent, int row, int extraColumn, int role = Qt::DisplayRole) const = 0;

    // KF6 TODO: add extraColumnFlags() virtual method

    /**
     * This method is called by setData() for extra columns.
     * Reimplement this method to set the data for the extra columns, if editing is supported.
     * Remember to call extraColumnDataChanged() after changing the data storage.
     * The default implementation returns false.
     */
    virtual bool setExtraColumnData(const QModelIndex &parent, int row, int extraColumn, const QVariant &data, int role = Qt::EditRole);

    /**
     * This method can be called by your derived class when the data in an extra column has changed.
     * The use case is data that changes "by itself", unrelated to setData.
     */
    void extraColumnDataChanged(const QModelIndex &parent, int row, int extraColumn, const QVector<int> &roles);

    /**
     * Returns the extra column number (0, 1, ...) for a given column number of the proxymodel.
     * This basically means subtracting the amount of columns in the source model.
     */
    int extraColumnForProxyColumn(int proxyColumn) const;
    /**
     * Returns the proxy column number for a given extra column number (starting at 0).
     * This basically means adding the amount of columns in the source model.
     */
    int proxyColumnForExtraColumn(int extraColumn) const;

    // Implementation
    /// @reimp
    void setSourceModel(QAbstractItemModel *model) override;
    /// @reimp
    QModelIndex mapToSource(const QModelIndex &proxyIndex) const override;
    /// @reimp
    QItemSelection mapSelectionToSource(const QItemSelection &selection) const override;
    /// @reimp
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    /// @reimp
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    /// @reimp
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    /// @reimp
    QModelIndex sibling(int row, int column, const QModelIndex &idx) const override;
    /// @reimp
    QModelIndex buddy(const QModelIndex &index) const override;
    /// @reimp
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    /// @reimp
    bool hasChildren(const QModelIndex &index) const override;
    /// @reimp
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    /// @reimp
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    /// @reimp
    QModelIndex parent(const QModelIndex &child) const override;

private:
    Q_DECLARE_PRIVATE(KExtraColumnsProxyModel)
    Q_PRIVATE_SLOT(d_func(), void _ec_sourceLayoutAboutToBeChanged(const QList<QPersistentModelIndex> &, QAbstractItemModel::LayoutChangeHint))
    Q_PRIVATE_SLOT(d_func(), void _ec_sourceLayoutChanged(const QList<QPersistentModelIndex> &, QAbstractItemModel::LayoutChangeHint))
    const QScopedPointer<KExtraColumnsProxyModelPrivate> d_ptr;
};

#endif
