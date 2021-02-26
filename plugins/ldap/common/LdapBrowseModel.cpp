/*
 * LdapModel.cpp - item model for browsing LDAP directories
 *
 * Copyright (c) 2019-2021 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "LdapClient.h"
#include "LdapConfiguration.h"
#include "LdapBrowseModel.h"

#if defined(QT_TESTLIB_LIB) && QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
#include <QAbstractItemModelTester>
#endif

// clazy:excludeall=rule-of-three

class LdapBrowseModelNode {
public:
	enum Type {
		Root,
		DN,
		Attribute
	};

	explicit LdapBrowseModelNode( Type type, const QString& name, LdapBrowseModelNode *parent ) :
		m_parent( parent ),
		m_name( name ),
		m_type( type ),
		m_populated( false )
	{
		if( type == Attribute )
		{
			m_populated = true;
		}
	}

	~LdapBrowseModelNode()
	{
		qDeleteAll(m_childItems);
	}

	Q_DISABLE_COPY(LdapBrowseModelNode)
	Q_DISABLE_MOVE(LdapBrowseModelNode)

	Type type() const
	{
		return m_type;
	}

	LdapBrowseModelNode* parent() const
	{
		return m_parent;
	}

	int row() const
	{
		if( m_parent )
		{
			return m_parent->children().indexOf( const_cast<LdapBrowseModelNode *>(this) );
		}

		return 0;
	}

	void setPopulated( bool populated )
	{
		m_populated = populated;
	}

	bool isPopulated() const
	{
		return m_populated;
	}

	void appendChild( LdapBrowseModelNode* item )
	{
		m_childItems.append( item );
	}

	const QList<LdapBrowseModelNode *>& children() const
	{
		return m_childItems;
	}

	const QString& name() const
	{
		return m_name;
	}

private:
	LdapBrowseModelNode* m_parent;
	QList<LdapBrowseModelNode *> m_childItems;
	QString m_name;
	Type m_type;
	bool m_populated;

};



LdapBrowseModel::LdapBrowseModel( Mode mode, const LdapConfiguration& configuration, QObject* parent ) :
	QAbstractItemModel( parent ),
	m_mode( mode ),
	m_client( new LdapClient( configuration, QUrl(), this ) ),
	m_root( new Node( Node::Root, {}, nullptr ) ),
	m_objectIcon( QStringLiteral(":/core/document-open.png") ),
	m_ouIcon( QStringLiteral( ":/ldap/folder-stash.png") ),
	m_attributeIcon( QStringLiteral(":/ldap/attribute.png") )
{
	populateRoot();

#if defined(QT_TESTLIB_LIB) && QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
	new QAbstractItemModelTester( this, QAbstractItemModelTester::FailureReportingMode::Warning, this );
#endif
}



LdapBrowseModel::~LdapBrowseModel()
{
	delete m_root;
	delete m_client;
}



QModelIndex LdapBrowseModel::index( int row, int col, const QModelIndex& parent ) const
{
	auto childNode = toNode( parent )->children().value( row );
	if( childNode )
	{
		return createIndex( row, col, childNode );
	}

	return {};
}



QModelIndex LdapBrowseModel::parent( const QModelIndex& child ) const
{
	if( child.isValid() == false )
	{
		return {};
	}

	auto childItem = static_cast<Node *>( child.internalPointer() );
	auto parentItem = childItem->parent();

	if( parentItem == m_root )
	{
		return {};
	}

	return createIndex( parentItem->row(), 0, parentItem );
}



QVariant LdapBrowseModel::data( const QModelIndex& index, int role ) const
{
	if( index.isValid() == false )
	{
		return {};
	}

	const auto node = toNode( index );

	switch( role )
	{
	case Qt::DisplayRole:
		return LdapClient::toRDNs( node->name() ).value( 0 );

	case Qt::DecorationRole:
		switch( node->type() )
		{
		case Node::Root: break;
		case Node::DN: return node->name().startsWith( QLatin1String("ou="), Qt::CaseInsensitive ) ? m_ouIcon : m_objectIcon;
		case Node::Attribute: return m_attributeIcon;
		}
		break;

	case ItemNameRole:
		return node->name();

	default:
		break;
	}

	return {};
}



Qt::ItemFlags LdapBrowseModel::flags( const QModelIndex& index ) const
{
	if( index.isValid() == false )
	{
		return QAbstractItemModel::flags(index);
	}

	return QAbstractItemModel::flags(index) | Qt::ItemIsSelectable;
}



int LdapBrowseModel::columnCount( const QModelIndex& parent ) const
{
	Q_UNUSED(parent);
	return 1;
}



int LdapBrowseModel::rowCount( const QModelIndex& parent ) const
{
	if( parent.column() >  0 )
	{
		return 0;
	}

	return toNode( parent )->children().count();
}



bool LdapBrowseModel::hasChildren( const QModelIndex& parent ) const
{
	auto parentNode = toNode( parent );

	if( parent.isValid() && parentNode && parentNode->isPopulated() )
	{
		return parentNode->children().isEmpty() == false;
	}

	return true;
}



bool LdapBrowseModel::canFetchMore( const QModelIndex& parent ) const
{
	return toNode( parent )->isPopulated() == false;
}



void LdapBrowseModel::fetchMore( const QModelIndex& parent )
{
	populateNode( parent );
}



QModelIndex LdapBrowseModel::dnToIndex( const QString& dn )
{
	auto rdns = LdapClient::toRDNs( dn );

	auto node = m_root;
	QModelIndex index;
	QStringList fullDn;

	while( rdns.isEmpty() == false )
	{
		populateNode( index );

		fullDn.prepend( rdns.takeLast() );
		const auto currentDn = fullDn.join( QLatin1Char(',') ).toLower();

		int row = 0;
		const int count = node->children().count();

		for( auto child : node->children() )
		{
			if( child->name().toLower() == currentDn )
			{
				node = child;
				index = createIndex( row, 0, node );
				break;
			}
			++row;
		}

		if( row >= count )
		{
			return index;
		}
	}

	return index;
}



QString LdapBrowseModel::baseDn() const
{
	return m_client->baseDn();
}



void LdapBrowseModel::populateRoot() const
{
	QStringList namingContexts;
	const auto baseDn = m_client->configuration().baseDn();

	if( baseDn.isEmpty() || m_mode == BrowseBaseDN )
	{
		namingContexts.append( m_client->queryNamingContexts() );
		namingContexts.append( m_client->queryNamingContexts( QStringLiteral("namingContexts") ) );
		namingContexts.append( m_client->queryNamingContexts( QStringLiteral("defaultNamingContext") ) );
		namingContexts.removeDuplicates();

		// remove sub naming contexts
		for( const auto& context : namingContexts )
		{
			if( context.isEmpty() == false )
			{
				namingContexts.replaceInStrings( QRegExp( QStringLiteral(".*,%1").arg( context ) ), {} );
			}
		}
		namingContexts.removeAll( {} );
		namingContexts.sort();
	}
	else
	{
		namingContexts.append( baseDn );
	}

	for( const auto& namingContext : namingContexts )
	{
		auto parent = m_root;
		QStringList fullDn;
		const auto dns = namingContext.split( QLatin1Char(',') );
#if QT_VERSION < 0x050600
#warning Building compat code for unsupported version of Qt
		using QStringListReverseIterator = std::reverse_iterator<QStringList::const_iterator>;
		for( auto it = QStringListReverseIterator(dns.cend()),
			 end = QStringListReverseIterator(dns.cbegin()); it != end; ++it )
#else
		for( auto it = dns.crbegin(), end = dns.crend(); it != end; ++it )
#endif
		{
			fullDn.prepend( *it );
			auto node = new Node( Node::DN, fullDn.join( QLatin1Char(',') ), parent );
			parent->appendChild( node );
			parent = node;
		}
	}

	m_root->setPopulated( true );
}



void LdapBrowseModel::populateNode( const QModelIndex& parent )
{
	auto node = toNode( parent );

	if( node->isPopulated() == false )
	{
		auto dns = m_client->queryDistinguishedNames( node->name(), {}, LdapClient::Scope::One );
		dns.sort();

		QStringList attributes;

		if( m_mode == BrowseAttributes )
		{
			attributes = m_client->queryObjectAttributes( node->name() );
			attributes.sort();
		}

		const auto itemCount = ( dns + attributes ).size();

		if( itemCount > 0 )
		{
			beginInsertRows( parent, 0, itemCount - 1 );

			for( const auto& dn : dns )
			{
				node->appendChild( new Node( Node::DN, dn, node ) );
			}

			for( const auto& attribute : qAsConst(attributes) )
			{
				node->appendChild( new Node( Node::Attribute, attribute, node ) );
			}

			endInsertRows();

			Q_EMIT layoutChanged();
		}

		node->setPopulated( true );
	}
}



LdapBrowseModel::Node*LdapBrowseModel::toNode( const QModelIndex& index ) const
{
	return index.isValid() ? static_cast<Node *>( index.internalPointer() ) : m_root;
}
