////////////////////////////////////////////////////////////////////////////////////////////////////
// Noesis Engine - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsApp/NotifyPropertyChangedBase.h>
#include <NsCore/ReflectionImplement.h>


using namespace Noesis;
using namespace NoesisApp;


////////////////////////////////////////////////////////////////////////////////////////////////////
PropertyChangedEventHandler& NotifyPropertyChangedBase::PropertyChanged()
{
    return _propertyChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void NotifyPropertyChangedBase::OnPropertyChanged(const char* name)
{
    if (_propertyChanged)
    {
        _propertyChanged(this, PropertyChangedEventArgs(Symbol(name)));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION(NotifyPropertyChangedBase)
{
    NsImpl<INotifyPropertyChanged>();
}

NS_END_COLD_REGION
