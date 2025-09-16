/*======================================================================================================================
 * trees/TreeNodeTraits.h is part of Brewtarget, and is copyright the following authors 2024-2025:
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
#ifndef TREES_TREENODETRAITS_H
#define TREES_TREENODETRAITS_H
#pragma once

#include "config.h"
#include "model/Boil.h"
#include "model/BoilStep.h"
#include "model/Equipment.h"
#include "model/Fermentable.h"
#include "model/Fermentation.h"
#include "model/FermentationStep.h"
#include "model/Folder.h"
#include "model/Hop.h"
#include "model/Mash.h"
#include "model/MashStep.h"
#include "model/Misc.h"
#include "model/Salt.h"
#include "model/Style.h"
#include "model/Water.h"
#include "model/Yeast.h"

namespace {
   /**
    * \brief When we have an optional property, we can't just hand a std::optional type back to Qt, so we handle both
    *        "set" and "unset" cases separately.
    *
    * \param displayString - if supplied, is used to convert value to a string before converting to a variant
    * \param value
    * \param units Units to which to convert value (assuming it is in canonical units in the same unit system)
    */
   template<typename T>
   QVariant qVariantFromOptional(std::optional<T> value) {
      if (value) {
         return QVariant::fromValue(*value);
      }
      return QVariant{};
   }
   template<typename T>
   QVariant qVariantFromOptional(QString const & displayString,
                                 std::optional<T> value,
                                 Measurement::Unit const & units) {
      if (value) {
         return displayString.arg(Measurement::displayAmount(Measurement::Amount{units.fromCanonical(*value), units}));
      }
      return displayString.arg("-");
   }
}

/**
 * \brief See comment in qtModels/tableModels/TableModelBase.h for why we use a traits class to allow the following attributes
 *        from each \c Derived class to be accessible in \c TreeNodeBase:
 *           - \c ColumnIndex        = class enum for the columns of this node type
 *           - \c NumberOfColumns    = number of entries in the above.  (Yes, it is a bit frustrating that we cannot
 *                                     easily deduce the number of values of a class enum.  Hopefully this will change
 *                                     in future versions of C++.)
 *           - \c NodeClassifier     = \c TreeNodeClassifier for this node type
 *           - \c ParentPtrTypes     = std::variant of raw pointers to valid parent types
 *           - \c ChildPtrTypes      = std::variant of shared_ptrs to valid child types (or
 *                                     std::variant<std::monostate> if no children are allowed).
 *           - \c DragNDropMimeType  = used with drag-and-drop to determine which things can be dropped where.  See
 *                                     \c mimeAccepted properties in \c ui/mainWindow.ui.  Note that this type
 *                                     determines where a dragged item can be dropped.  Broadly:
 *                                       - Recipes, equipment and styles get dropped on the recipe pane
 *                                       - Folders will be handled by themselves
 *                                       - Most other things get dropped on the ingredients pane
 *                                       - TBD what to do about Water
 *                                       - BrewNotes can't be dropped anywhere
 *        We use smart pointers for children and raw pointers for parents because parents own their children (and not
 *        vice versa).  We use std::variant even in trees where all nodes have a single parent type because it
 *        simplifies the generic code.
 */
template<class NE, class TreeType = NE>
struct TreeNodeTraits;


template<class NE> class TreeFolderNode;
template<class NE> class TreeItemNode;

template <class NE> struct TreeNodeTraits<Folder, NE> {
   enum class ColumnIndex {
      // TBD: Not sure we need all these columns!
      Name    ,
      Path    ,
      FullPath,
   };
   static constexpr size_t NumberOfColumns = 3;
   static constexpr TreeNodeClassifier NodeClassifier = TreeNodeClassifier::Folder;
   using TreeType = NE;
   using ParentPtrTypes = std::variant<TreeFolderNode<NE> *>;
   using ChildPtrTypes = std::variant<std::shared_ptr<TreeFolderNode<NE>>, std::shared_ptr<TreeItemNode<NE>>>;
   static constexpr char const * DragNDropMimeType = DEF_CONFIG_MIME_PREFIX "-folder";

   static QVariant data(Folder const & folder, ColumnIndex const column) {
      switch (column) {
         case ColumnIndex::Name:
            return QVariant(folder.name());
         case ColumnIndex::Path:
            return QVariant(folder.path());
         case ColumnIndex::FullPath:
            return QVariant(folder.fullPath());
      }
      std::unreachable();
   }
};

template<> struct TreeNodeTraits<BrewNote, Recipe> {
   enum class ColumnIndex {
      BrewDate,
   };
   static constexpr size_t NumberOfColumns = 1;
   static constexpr TreeNodeClassifier NodeClassifier = TreeNodeClassifier::SecondaryItem;
   using TreeType = Recipe;
   using ParentPtrTypes = std::variant<TreeItemNode<Recipe> *>;
   using ChildPtrTypes = std::variant<std::monostate>;
   // BrewNotes can't be dropped anywhere, so there isn't anywhere in the program that accepts drops with this MIME type
   static constexpr char const * DragNDropMimeType = DEF_CONFIG_MIME_PREFIX "-brewnote";

   static QVariant data(BrewNote const & brewNote, ColumnIndex const column) {
      // I know this is a bit overkill when we only have one column, but I prefer to keep the same code structure for
      // all node types - in case we decide to add more columns in future.
      switch (column) {
         case ColumnIndex::BrewDate:
            return QVariant(brewNote.brewDate_short());
      }
      std::unreachable();
   }
};

template<> struct TreeNodeTraits<Recipe, Recipe> {
   enum class ColumnIndex {
      Name             ,
      NumberOfAncestors,
      BrewDate         ,
      Style            ,
   };
   static constexpr size_t NumberOfColumns = 4;
   static constexpr TreeNodeClassifier NodeClassifier = TreeNodeClassifier::PrimaryItem;
   using TreeType = Recipe;
   using ParentPtrTypes = std::variant<TreeFolderNode<Recipe> *, TreeItemNode<Recipe> *>;
   using ChildPtrTypes = std::variant<std::shared_ptr<TreeItemNode<BrewNote>>, std::shared_ptr<TreeItemNode<Recipe>>>;
   static constexpr char const * DragNDropMimeType = DEF_CONFIG_MIME_PREFIX "-recipe";

   static QString getRootName() { return Recipe::tr("Recipes"); }

   static QVariant data(Recipe const & recipe, ColumnIndex const column) {
      switch (column) {
         case ColumnIndex::Name:
            return QVariant(recipe.name());
         case ColumnIndex::NumberOfAncestors:
            return QVariant(recipe.ancestors().size());
         case ColumnIndex::BrewDate:
            return recipe.date() ? Localization::displayDateUserFormated(*recipe.date()) : QVariant();
         case ColumnIndex::Style:
            return recipe.style() ? QVariant(recipe.style()->name()) : QVariant();
      }
      std::unreachable();
   }
};

template<> struct TreeNodeTraits<Equipment, Equipment> {
   enum class ColumnIndex {
      Name    ,
      BoilTime,
   };
   static constexpr size_t NumberOfColumns = 2;
   static constexpr TreeNodeClassifier NodeClassifier = TreeNodeClassifier::PrimaryItem;
   using TreeType = Equipment;
   using ParentPtrTypes = std::variant<TreeFolderNode<Equipment> *>;
   using ChildPtrTypes = std::variant<std::monostate>;
   //
   // Although it seems odd for Equipment to have a drag-and-drop MIME type of recipe, it is intentional.  This means an
   // Equipment can be dropped on the recipe pane (MainWindow::tabWidget_recipeView).
   //
   static constexpr char const * DragNDropMimeType = DEF_CONFIG_MIME_PREFIX "-recipe";

   static QString getRootName() { return Equipment::tr("Equipments"); }

   static QVariant data(Equipment const & equipment, ColumnIndex const column) {
      switch (column) {
         case ColumnIndex::Name:
            return QVariant(equipment.name());
         case ColumnIndex::BoilTime:
            return QVariant::fromValue(equipment.boilTime_min());
      }
      std::unreachable();
   }
};

template<> struct TreeNodeTraits<Fermentable, Fermentable> {
   enum class ColumnIndex {
      Name ,
      Type ,
      Color,
   };
   static constexpr size_t NumberOfColumns = 3;
   static constexpr TreeNodeClassifier NodeClassifier = TreeNodeClassifier::PrimaryItem;
   using TreeType = Fermentable;
   using ParentPtrTypes = std::variant<TreeFolderNode<Fermentable> *>;
   using ChildPtrTypes = std::variant<std::monostate>;
   // Fermentables and other ingredients can be dropped on MainWindow::tabWidget_ingredients
   static constexpr char const * DragNDropMimeType = DEF_CONFIG_MIME_PREFIX "-ingredient";

   static QString getRootName() { return Fermentable::tr("Fermentables"); }

   static QVariant data(Fermentable const & fermentable, ColumnIndex const column) {
      switch (column) {
         case ColumnIndex::Name:
            return QVariant(fermentable.name());
         case ColumnIndex::Type:
            return QVariant(Fermentable::typeDisplayNames[fermentable.type()]);
         case ColumnIndex::Color:
            return QVariant(Measurement::displayAmount(Measurement::Amount{fermentable.color_srm(),
                                                                           Measurement::Units::srm}, 0));
      }
      std::unreachable();
   }

};

template<> struct TreeNodeTraits<Hop, Hop> {
   enum class ColumnIndex {
      Name    ,
      Form    ,
      AlphaPct, // % Alpha Acid
      Origin  , // Country of origin
   };
   static constexpr size_t NumberOfColumns = 4;
   static constexpr TreeNodeClassifier NodeClassifier = TreeNodeClassifier::PrimaryItem;
   using TreeType = Hop;
   using ParentPtrTypes = std::variant<TreeFolderNode<Hop> *>;
   using ChildPtrTypes = std::variant<std::monostate>;
   static constexpr char const * DragNDropMimeType = DEF_CONFIG_MIME_PREFIX "-ingredient";

   static QString getRootName() { return Hop::tr("Hops"); }

   static QVariant data(Hop const & hop, ColumnIndex const column) {
      switch (column) {
         case ColumnIndex::Name:
            return QVariant(hop.name());
         case ColumnIndex::Form:
            return QVariant(Hop::formDisplayNames[hop.form()]);
         case ColumnIndex::AlphaPct:
            return QVariant(hop.alpha_pct());
         case ColumnIndex::Origin:
            return QVariant(hop.origin());
      }
      std::unreachable();
   }
};

template<> struct TreeNodeTraits<MashStep, Mash> {
   enum class ColumnIndex {
      Name        ,
      Time        ,
//      Type        ,
//      Amount      ,
//      InfusionTemp,
//      TargetTemp  ,
   };
   static constexpr size_t NumberOfColumns = 2;
   static constexpr TreeNodeClassifier NodeClassifier = TreeNodeClassifier::SecondaryItem;
   using TreeType = Mash;
   using ParentPtrTypes = std::variant<TreeItemNode<Mash> *>;
   using ChildPtrTypes = std::variant<std::monostate>;
   // MashSteps can't be dropped anywhere, so there isn't anywhere in the program that accepts drops with this MIME type
   static constexpr char const * DragNDropMimeType = DEF_CONFIG_MIME_PREFIX "-MashStep";

   static QVariant data(MashStep const & mashStep, ColumnIndex const column) {
      switch (column) {
         case ColumnIndex::Name        : return QVariant(mashStep.name         ());
         case ColumnIndex::Time        : return MashStep::tr("%1 mins").arg(mashStep.stepTime_mins());
//         case ColumnIndex::Type        : return QVariant(mashStep.type         ());
//         case ColumnIndex::Amount      : return QVariant(mashStep.amount_l     ());
//         case ColumnIndex::InfusionTemp: return QVariant(mashStep.infuseTemp_c ());
//         case ColumnIndex::TargetTemp  : return QVariant(mashStep.startTemp_c  ());
      }
      std::unreachable();
   }
};

template<> struct TreeNodeTraits<Mash, Mash> {
   enum class ColumnIndex {
      Name      ,
      TotalWater,
      TotalTime ,
   };
   static constexpr size_t NumberOfColumns = 3;
   static constexpr TreeNodeClassifier NodeClassifier = TreeNodeClassifier::PrimaryItem;
   using TreeType = Mash;
   using ParentPtrTypes = std::variant<TreeFolderNode<Mash> *>;
   using ChildPtrTypes = std::variant<std::shared_ptr<TreeItemNode<MashStep>>>;
   static constexpr char const * DragNDropMimeType = DEF_CONFIG_MIME_PREFIX "-mash";

   static QString getRootName() { return Mash::tr("Mash Profiles"); }

   static QVariant data(Mash const & mash, ColumnIndex const column) {
      switch (column) {
         case ColumnIndex::Name:
            return QVariant(mash.name());
         case ColumnIndex::TotalWater:
            return QVariant(Measurement::displayAmount(Measurement::Amount{mash.totalMashWater_l(),
                                                                           Measurement::Units::liters}, 0));
         case ColumnIndex::TotalTime:
            return MashStep::tr("%1 mins").arg(mash.totalTime_mins());
      }
      std::unreachable();
   }
};

template<> struct TreeNodeTraits<BoilStep, Boil> {
   enum class ColumnIndex {
      Name        ,
      StepTime    ,
//      StartTemp   ,
//      RampTime    ,
//      EndTemp     ,
//      StartAcidity,
//      EndAcidity  ,
//      StartGravity,
//      EndGravity  ,
//      ChillingType,
   };
   static constexpr size_t NumberOfColumns = 1;
   static constexpr TreeNodeClassifier NodeClassifier = TreeNodeClassifier::SecondaryItem;
   using TreeType = Boil;
   using ParentPtrTypes = std::variant<TreeItemNode<Boil> *>;
   using ChildPtrTypes = std::variant<std::monostate>;
   // BoilSteps can't be dropped anywhere, so there isn't anywhere in the program that accepts drops with this MIME type
   static constexpr char const * DragNDropMimeType = DEF_CONFIG_MIME_PREFIX "-BoilStep";

   static QVariant data(BoilStep const & boilStep, ColumnIndex const column) {
      switch (column) {
         case ColumnIndex::Name        : return QVariant::fromValue(boilStep.name           ());
         case ColumnIndex::StepTime    : return qVariantFromOptional(BoilStep::tr("%1 mins"),
                                                                     boilStep.stepTime_mins(),
                                                                     Measurement::Units::minutes);
//         case ColumnIndex::StartTemp   : return QVariant::fromValue(boilStep.startTemp_c    ());
//         case ColumnIndex::RampTime    : return QVariant::fromValue(boilStep.rampTime_mins  ());
//         case ColumnIndex::EndTemp     : return QVariant::fromValue(boilStep.endTemp_c      ());
//         case ColumnIndex::StartAcidity: return QVariant::fromValue(boilStep.startAcidity_pH());
//         case ColumnIndex::EndAcidity  : return QVariant::fromValue(boilStep.endAcidity_pH  ());
//         case ColumnIndex::StartGravity: return QVariant::fromValue(boilStep.startGravity_sg());
//         case ColumnIndex::EndGravity  : return QVariant::fromValue(boilStep.  endGravity_sg());
//         case ColumnIndex::ChillingType: return QVariant::fromValue(BoilStep::chillingTypeDisplayNames[boilStep.chillingType   ()]);
      }
      std::unreachable();
   }
};

template<> struct TreeNodeTraits<Boil, Boil> {
   enum class ColumnIndex {
      Name              ,
      PreBoilSize       ,
      LengthOfBoilProper,
   };
   static constexpr size_t NumberOfColumns = 2;
   static constexpr TreeNodeClassifier NodeClassifier = TreeNodeClassifier::PrimaryItem;
   using TreeType = Boil;
   using ParentPtrTypes = std::variant<TreeFolderNode<Boil> *>;
   using ChildPtrTypes = std::variant<std::shared_ptr<TreeItemNode<BoilStep>>>;
   static constexpr char const * DragNDropMimeType = DEF_CONFIG_MIME_PREFIX "-boil";

   static QString getRootName() { return Boil::tr("Boil Profiles"); }

   static QVariant data(Boil const & boil, ColumnIndex const column) {
      switch (column) {
         case ColumnIndex::Name:
            return QVariant(boil.name());
         case ColumnIndex::PreBoilSize:
         {
            std::optional<double> const preBoilSize_l = boil.preBoilSize_l();
            if (!preBoilSize_l) {
               return QVariant{};
            }
            return QVariant(Measurement::displayAmount(Measurement::Amount{*preBoilSize_l,
                                                                           Measurement::Units::liters}, 0));
         }
         case ColumnIndex::LengthOfBoilProper:
            return QVariant::fromValue(boil.boilTime_mins());
      }
      std::unreachable();
   }
};

template<> struct TreeNodeTraits<FermentationStep, Fermentation> {
   enum class ColumnIndex {
      Name     ,
      Time     ,
//      StartTemp,
//      EndTemp  ,
   };
   static constexpr size_t NumberOfColumns = 2;
   static constexpr TreeNodeClassifier NodeClassifier = TreeNodeClassifier::SecondaryItem;
   using TreeType = Fermentation;
   using ParentPtrTypes = std::variant<TreeItemNode<Fermentation> *>;
   using ChildPtrTypes = std::variant<std::monostate>;
   // FermentationSteps can't be dropped anywhere, so there isn't anywhere in the program that accepts drops with this MIME type
   static constexpr char const * DragNDropMimeType = DEF_CONFIG_MIME_PREFIX "-FermentationStep";

   static QVariant data(FermentationStep const & fermentationStep, ColumnIndex const column) {
      switch (column) {
         case ColumnIndex::Name        : return QVariant(fermentationStep.name         ());
         case ColumnIndex::Time        : return qVariantFromOptional(FermentationStep::tr("%1 days"),
                                                                     fermentationStep.stepTime_mins(),
                                                                     Measurement::Units::days);
//         case ColumnIndex::StartTemp    : return QVariant(fermentationStep.startTemp_c         ());
//         case ColumnIndex::EndTemp      : return QVariant(fermentationStep.endTemp_c     ());
      }
      std::unreachable();
   }
};

template<> struct TreeNodeTraits<Fermentation, Fermentation> {
   enum class ColumnIndex {
      Name       ,
      Description,
   };
   static constexpr size_t NumberOfColumns = 2;
   static constexpr TreeNodeClassifier NodeClassifier = TreeNodeClassifier::PrimaryItem;
   using TreeType = Fermentation;
   using ParentPtrTypes = std::variant<TreeFolderNode<Fermentation> *>;
   using ChildPtrTypes = std::variant<std::shared_ptr<TreeItemNode<FermentationStep>>>;
   static constexpr char const * DragNDropMimeType = DEF_CONFIG_MIME_PREFIX "-fermentation";

   static QString getRootName() { return Fermentation::tr("Fermentation Profiles"); }

   static QVariant data(Fermentation const & fermentation, ColumnIndex const column) {
      switch (column) {
         case ColumnIndex::Name:
            return QVariant(fermentation.name());
         case ColumnIndex::Description:
            return QVariant::fromValue(fermentation.description());
      }
      std::unreachable();
   }
};

template<> struct TreeNodeTraits<Misc, Misc> {
   enum class ColumnIndex {
      Name,
      Type,
   };
   static constexpr size_t NumberOfColumns = 2;
   static constexpr TreeNodeClassifier NodeClassifier = TreeNodeClassifier::PrimaryItem;
   using TreeType = Misc;
   using ParentPtrTypes = std::variant<TreeFolderNode<Misc> *>;
   using ChildPtrTypes = std::variant<std::monostate>;
   static constexpr char const * DragNDropMimeType = DEF_CONFIG_MIME_PREFIX "-ingredient";

   static QString getRootName() { return Misc::tr("Miscellaneous"); }

   static QVariant data(Misc const & misc, ColumnIndex const column) {
      switch (column) {
         case ColumnIndex::Name:
            return QVariant(misc.name());
         case ColumnIndex::Type:
            return QVariant(Misc::typeDisplayNames[misc.type()]);
      }
      std::unreachable();
   }
};

template<> struct TreeNodeTraits<Yeast, Yeast> {
   enum class ColumnIndex {
      // It's tempting to put Laboratory first, and have it at the first column, but it messes up the way the folders
      // work if the first column isn't Name
      Name,
      Laboratory,
      ProductId,
      Type,
      Form,
   };
   static constexpr size_t NumberOfColumns = 5;
   static constexpr TreeNodeClassifier NodeClassifier = TreeNodeClassifier::PrimaryItem;
   using TreeType = Yeast;
   using ParentPtrTypes = std::variant<TreeFolderNode<Yeast> *>;
   using ChildPtrTypes = std::variant<std::monostate>;
   static constexpr char const * DragNDropMimeType = DEF_CONFIG_MIME_PREFIX "-ingredient";

   static QString getRootName() { return Yeast::tr("Yeasts"); }

   static QVariant data(Yeast const & yeast, ColumnIndex const column) {
      switch (column) {
         case ColumnIndex::Name:
            return QVariant(yeast.name());
         case ColumnIndex::Laboratory:
            return QVariant(yeast.laboratory());
         case ColumnIndex::ProductId:
            return QVariant(yeast.productId());
         case ColumnIndex::Type:
            return QVariant(Yeast::typeDisplayNames[yeast.type()]);
         case ColumnIndex::Form:
            return QVariant(Yeast::formDisplayNames[yeast.form()]);
      }
      std::unreachable();
   }
};

template<> struct TreeNodeTraits<Salt, Salt> {
   enum class ColumnIndex {
      Name       ,
      Type       ,
      IsAcid     ,
      PercentAcid,
   };
   static constexpr size_t NumberOfColumns = 4;
   static constexpr TreeNodeClassifier NodeClassifier = TreeNodeClassifier::PrimaryItem;
   using TreeType = Salt;
   using ParentPtrTypes = std::variant<TreeFolderNode<Salt> *>;
   using ChildPtrTypes = std::variant<std::monostate>;
   static constexpr char const * DragNDropMimeType = DEF_CONFIG_MIME_PREFIX "-recipe";

   static QString getRootName() { return Salt::tr("Salts"); }

   static QVariant data(Salt const & salt, ColumnIndex const column) {
      switch (column) {
         case ColumnIndex::Name:
            return QVariant(salt.name());
         case ColumnIndex::Type:
            return QVariant(Salt::typeDisplayNames[salt.type()]);
         case ColumnIndex::IsAcid:
            return QVariant(salt.isAcid());
         case ColumnIndex::PercentAcid:
            return qVariantFromOptional(salt.percentAcid());
      }
      std::unreachable();
   }
};

template<> struct TreeNodeTraits<Style, Style> {
   enum class ColumnIndex {
      Name          ,
      Category      ,
      CategoryNumber,
      CategoryLetter,
      StyleGuide    ,
   };
   static constexpr size_t NumberOfColumns = 5;
   static constexpr TreeNodeClassifier NodeClassifier = TreeNodeClassifier::PrimaryItem;
   using TreeType = Style;
   using ParentPtrTypes = std::variant<TreeFolderNode<Style> *>;
   using ChildPtrTypes = std::variant<std::monostate>;
   static constexpr char const * DragNDropMimeType = DEF_CONFIG_MIME_PREFIX "-recipe";

   static QString getRootName() { return Style::tr("Styles"); }

   static QVariant data(Style const & style, ColumnIndex const column) {
      switch (column) {
         case ColumnIndex::Name:
            return QVariant(style.name());
         case ColumnIndex::Category:
            return QVariant(style.category());
         case ColumnIndex::CategoryNumber:
            return QVariant(style.categoryNumber());
         case ColumnIndex::CategoryLetter:
            return QVariant(style.styleLetter());
         case ColumnIndex::StyleGuide:
            return QVariant(style.styleGuide());
      }
      std::unreachable();
   }
};

template<> struct TreeNodeTraits<Water, Water> {
   enum class ColumnIndex {
      Name       ,
      Calcium    ,
      Bicarbonate,
      Sulfate    ,
      Chloride   ,
      Sodium     ,
      Magnesium  ,
      pH         ,
   };
   static constexpr size_t NumberOfColumns = 8;
   static constexpr TreeNodeClassifier NodeClassifier = TreeNodeClassifier::PrimaryItem;
   using TreeType = Water;
   using ParentPtrTypes = std::variant<TreeFolderNode<Water> *>;
   using ChildPtrTypes = std::variant<std::monostate>;
   static constexpr char const * DragNDropMimeType = DEF_CONFIG_MIME_PREFIX "-ingredient";

   static QString getRootName() { return Water::tr("Waters"); }

   static QVariant data(Water const & water, ColumnIndex const column) {
      switch (static_cast<ColumnIndex>(column)) {
         case ColumnIndex::Name:
            return QVariant(water.name());
         case ColumnIndex::Calcium:
            return QVariant(water.calcium_ppm());
         case ColumnIndex::Bicarbonate:
            return QVariant(water.bicarbonate_ppm());
         case ColumnIndex::Sulfate:
            return QVariant(water.sulfate_ppm());
         case ColumnIndex::Chloride:
            return QVariant(water.chloride_ppm());
         case ColumnIndex::Sodium:
            return QVariant(water.sodium_ppm());
         case ColumnIndex::Magnesium:
            return QVariant(water.magnesium_ppm());
         case ColumnIndex::pH:
            return water.ph() ? QVariant(*water.ph()) : QVariant();
      }
      std::unreachable();
   }
};


#endif
