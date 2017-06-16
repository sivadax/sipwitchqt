/**
 ** Copyright 2017 Tycho Softworks.
 **
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include "compiler.hpp"
#include "database.hpp"

class ProviderQuery final : public Request
{
    Q_DISABLE_COPY(ProviderQuery)
public:
/*	static inline void reload() {
		Database::providerList(new ProviderQuery());
	}

	static inline void update(const QString& contact) {
		Database::providerGet(new ProviderQuery(contact));
	}
*/

private:
    ProviderQuery();
    ProviderQuery(const QString& contact);
	
private slots:
    void reload(ErrorResult err, const QVariantHash& keys, const QList<QSqlRecord>& records);
    void update(ErrorResult err, const QVariantHash& keys, const QList<QSqlRecord>& records);
};
