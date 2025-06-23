/*======================================================================================================================
 * InventoryWindow.cpp is part of Brewtarget, and is copyright the following authors 2025:
 *   • Matt Young <mfsy@yahoo.com>
 *
 * Brewtarget is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * Brewtarget is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 =====================================================================================================================*/
#include "InventoryWindow.h"

InventoryWindow::InventoryWindow(QWidget * parent) :
   QDialog{},
   Ui::inventoryWindow{} {

   // This sets various things from the inventoryWindow.ui file, including window title
   this->setupUi(this);
   return;
}

InventoryWindow::~InventoryWindow() = default;
