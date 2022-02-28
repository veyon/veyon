/*
    SPDX-FileCopyrightText: 2009 Stephen Kelly <steveire@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KRECURSIVEFILTERPROXYMODEL_H
#define KRECURSIVEFILTERPROXYMODEL_H

#include <QScopedPointer>

#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)

#include <QSortFilterProxyModel>

class KRecursiveFilterProxyModelPrivate;

/**
  @class KRecursiveFilterProxyModel krecursivefilterproxymodel.h KRecursiveFilterProxyModel

  @brief Implements recursive filtering of models

  Until Qt 5.10, QSortFilterProxyModel did not recurse when invoking a filtering stage, so that
  if a particular row is filtered out, its children are not even checked to see if they match the filter.

  If you can depend on Qt >= 5.10, then just use QSortFilterProxyModel::setRecursiveFilteringEnabled(true),
  and you don't need to use KRecursiveFilterProxyModel.

  For example, given a source model:

  @verbatim
    - A
    - B
    - - C
    - - - D
    - - - - E
    - - - F
    - - G
    - - H
    - I
  @endverbatim

  If a QSortFilterProxyModel is used with a filter matching A, D, G and I, the QSortFilterProxyModel will contain

  @verbatim
    - A
    - I
  @endverbatim

  That is, even though D and G match the filter, they are not represented in the proxy model because B does not
  match the filter and is filtered out.

  The KRecursiveFilterProxyModel checks child indexes for filter matching and ensures that all matching indexes
  are represented in the model.

  In the above example, the KRecursiveFilterProxyModel will contain

  @verbatim
    - A
    - B
    - - C
    - - - D
    - - G
    - I
  @endverbatim

  That is, the leaves in the model match the filter, but not necessarily the inner branches.

  QSortFilterProxyModel provides the virtual method filterAcceptsRow to allow custom filter implementations.
  Custom filter implementations can be written for KRecuriveFilterProxyModel using the acceptRow virtual method.

  Note that using this proxy model is additional overhead compared to QSortFilterProxyModel as every index in the
  model must be visited and queried.

  @author Stephen Kelly <steveire@gmail.com>

  @since 4.5

  @deprecated since 5.65, use QSortFilterProxyModel::setRecursiveFilteringEnabled(true) instead. See detailed description.
*/
class KRecursiveFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    /**
      Constructor
    */
    explicit KRecursiveFilterProxyModel(QObject *parent = nullptr);

    /**
      Destructor
    */
    ~KRecursiveFilterProxyModel() override;

    /** @reimp */
    void setSourceModel(QAbstractItemModel *model) override;

    /**
     * @reimplemented
     */
    QModelIndexList match(const QModelIndex &start,
                          int role,
                          const QVariant &value,
                          int hits = 1,
                          Qt::MatchFlags flags = Qt::MatchFlags(Qt::MatchStartsWith | Qt::MatchWrap)) const override;

protected:
    /**
      Reimplement this method for custom filtering strategies.
    */
    virtual bool acceptRow(int sourceRow, const QModelIndex &sourceParent) const;

    /** @reimp */
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    const QScopedPointer<KRecursiveFilterProxyModelPrivate> d_ptr;

private:
    //@cond PRIVATE
    Q_DECLARE_PRIVATE(KRecursiveFilterProxyModel)

    Q_PRIVATE_SLOT(d_func(),
                   void sourceDataChanged(const QModelIndex &source_top_left,
                                          const QModelIndex &source_bottom_right,
                                          const QVector<int> &roles = QVector<int>()))
    Q_PRIVATE_SLOT(d_func(), void sourceRowsAboutToBeInserted(const QModelIndex &source_parent, int start, int end))
    Q_PRIVATE_SLOT(d_func(), void sourceRowsInserted(const QModelIndex &source_parent, int start, int end))
    Q_PRIVATE_SLOT(d_func(), void sourceRowsAboutToBeRemoved(const QModelIndex &source_parent, int start, int end))
    Q_PRIVATE_SLOT(d_func(), void sourceRowsRemoved(const QModelIndex &source_parent, int start, int end))
    //@endcond
};

#endif

#endif
