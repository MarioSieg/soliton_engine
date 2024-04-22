////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include "MainWindow.xaml.h"

#include <NsCore/Version.h>
#include <NsCore/Factory.h>
#include <NsGui/Keyboard.h>
#include <NsGui/FreezableCollection.h>
#include <NsGui/Transform.h>
#include <NsGui/TransformGroup.h>
#include <NsGui/ScaleTransform.h>
#include <NsGui/TranslateTransform.h>
#include <NsGui/IntegrationAPI.h>
#include <NsGui/CompositeTransform3D.h>
#include <NsGui/ObservableCollection.h>
#include <NsGui/Border.h>
#include <NsGui/ItemsControl.h>
#include <NsGui/Stream.h>
#include <NsGui/Fonts.h>
#include <NsGui/StackPanel.h>
#include <NsGui/Brushes.h>
#include <NsGui/SolidColorBrush.h>
#include <NsGui/TextBlock.h>
#include <NsGui/Run.h>
#include <NsGui/LineBreak.h>
#include <NsGui/FontFamily.h>
#include <NsGui/Separator.h>
#include <NsGui/Image.h>
#include <NsGui/BitmapImage.h>
#include <NsGui/UIElementCollection.h>
#include <NsGui/SVG.h>
#include <NsGui/Canvas.h>
#include <NsDrawing/Thickness.h>
#include <NsApp/Application.h>


using namespace Noesis;
using namespace NoesisApp;
using namespace XamlPlayer;


////////////////////////////////////////////////////////////////////////////////////////////////////
MainWindow::MainWindow(): mRoot(0), mErrors(0), mContainer(0), mContainerScale(0),
    mContainerTranslate(0), mTransform3D(0), mZoom(1.0f), mDragging(false), mRotating(false)
{
    Initialized() += MakeDelegate(this, &MainWindow::OnInitialized);
    Loaded() += MakeDelegate(this, &MainWindow::OnLoaded);
    InitializeComponent();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
MainWindow::~MainWindow()
{
    // Restore default error handler so following errors don't try to use this
    SetErrorHandler(mDefaultErrorHandler);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static bool IsImage(const char* filename)
{
    return StrCaseEndsWith(filename, ".png") || StrCaseEndsWith(filename, ".jpg") ||
        StrCaseEndsWith(filename, ".tga") || StrCaseEndsWith(filename, ".bmp") ||
        StrCaseEndsWith(filename, ".gif");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static bool IsFont(const char* filename)
{
    return StrCaseEndsWith(filename, ".ttf") || StrCaseEndsWith(filename, ".otf");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static bool IsGradientBrush(const SVG::Brush& brush)
{
    return brush.type == SVG::Brush::Type::Linear || brush.type == SVG::Brush::Type::Radial;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void WriteBrush(BaseString& xaml, const SVG::Brush& brush)
{
    if (brush.mtx != Transform2::Identity())
    {
        if (brush.mappingMode == BrushMappingMode_RelativeToBoundingBox)
        {
            xaml.AppendFormat(" RelativeTransform=\"%g %g %g %g %g %g\"", brush.mtx[0][0],
                brush.mtx[0][1], brush.mtx[1][0], brush.mtx[1][1],
                brush.mtx[2][0], brush.mtx[2][1]);
        }
        else
        {
            xaml.AppendFormat(" Transform=\"%g %g %g %g %g %g\"", brush.mtx[0][0],
                brush.mtx[0][1], brush.mtx[1][0], brush.mtx[1][1],
                brush.mtx[2][0], brush.mtx[2][1]);
        }
    }

    if (brush.spreadMethod != GradientSpreadMethod_Pad)
    {
        if (brush.spreadMethod == GradientSpreadMethod_Reflect)
        {
            xaml.Append(" SpreadMethod=\"Reflect\"");
        }
        else if (brush.spreadMethod == GradientSpreadMethod_Repeat)
        {
            xaml.Append(" SpreadMethod=\"Repeat\"");
        }
    }

    if (brush.mappingMode != BrushMappingMode_RelativeToBoundingBox)
    {
        xaml.Append(" MappingMode=\"Absolute\"");
    }

    if (brush.opacity != 1.0f)
    {
        xaml.AppendFormat(" Opacity=\"%g\"", brush.opacity);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void WriteStops(BaseString& xaml, const SVG::Brush& brush)
{
    for (auto& stop : brush.stops)
    {
        Color c = Color::FromPackedBGRA(stop.color);
        String str = c.ToString();

        xaml.AppendFormat("        <GradientStop Offset=\"%g\" Color=\"%s\" />\n",
            stop.offset, str.Str());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void WriteLinearBrush(BaseString& xaml, const SVG::Brush& brush)
{
    xaml.Append("      <LinearGradientBrush");
    WriteBrush(xaml, brush);

    if (!IsZero(brush.linear.x1) || !IsZero(brush.linear.y1))
    {
        xaml.AppendFormat(" StartPoint=\"%g,%g\"", brush.linear.x1, brush.linear.y1);
    }

    if (!IsOne(brush.linear.x2) || !IsOne(brush.linear.y2))
    {
        xaml.AppendFormat(" EndPoint=\"%g,%g\"", brush.linear.x2, brush.linear.y2);
    }

    xaml.Append(">\n");
    WriteStops(xaml, brush);
    xaml.Append("      </LinearGradientBrush>\n");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void WriteRadialBrush(BaseString& xaml, const SVG::Brush& brush)
{
    xaml.Append("      <RadialGradientBrush");
    WriteBrush(xaml, brush);

    if (!AreClose(brush.radial.fx, 0.5f) || !AreClose(brush.radial.fy, 0.5f))
    {
        xaml.AppendFormat(" GradientOrigin=\"%g,%g\"", brush.radial.fx, brush.radial.fy);
    }

    if (!AreClose(brush.radial.cx, 0.5f) || !AreClose(brush.radial.cy, 0.5f))
    {
        xaml.AppendFormat(" Center=\"%g,%g\"", brush.radial.cx, brush.radial.cy);
    }

    if (!AreClose(brush.radial.r, 0.5f))
    {
        xaml.AppendFormat(" RadiusX=\"%g\" RadiusY=\"%g\"", brush.radial.r, brush.radial.r);
    }

    xaml.Append(">\n");
    WriteStops(xaml, brush);
    xaml.Append("      </RadialGradientBrush>\n");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void WriteBrushes(BaseString& xaml, const SVG::Shape& shape)
{
    if (IsGradientBrush(shape.fill))
    {
        xaml.Append("    <Path.Fill>\n");

        if (shape.fill.type == SVG::Brush::Type::Linear)
        {
            WriteLinearBrush(xaml, shape.fill);
        }
        else if (shape.fill.type == SVG::Brush::Type::Radial)
        {
            WriteRadialBrush(xaml, shape.fill);
        }

        xaml.Append("    </Path.Fill>\n");
    }

    if (IsGradientBrush(shape.stroke))
    {
        xaml.Append("    <Path.Stroke>\n");

        if (shape.stroke.type == SVG::Brush::Type::Linear)
        {
            WriteLinearBrush(xaml, shape.stroke);
        }
        else if (shape.stroke.type == SVG::Brush::Type::Radial)
        {
            WriteRadialBrush(xaml, shape.stroke);
        }

        xaml.Append("    </Path.Stroke>\n");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void WriteXaml(BaseString& xaml, const SVG::Image& image)
{
    xaml += "<Canvas xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\">\n";

    for (const SVG::Shape& shape : image.shapes)
    {
        xaml += "  <Path";
        xaml.AppendFormat(" Data=\"%s\"", shape.data.Str());

        if (shape.fill.type == SVG::Brush::Type::Solid)
        {
            Color c = Color::FromPackedBGRA(shape.fill.color);
            String str = c.ToString();
            xaml.AppendFormat(" Fill=\"%s\"", str.Str());
        }

        if (shape.stroke.type == SVG::Brush::Type::Solid)
        {
            Color c = Color::FromPackedBGRA(shape.stroke.color);
            String str = c.ToString();
            xaml.AppendFormat(" Stroke=\"%s\"", str.Str());
        }

        if (shape.thickness != 1.0f)
        {
            xaml.AppendFormat(" StrokeThickness=\"%g\"", shape.thickness);
        }

        if (!shape.dashArray.Empty())
        {
            xaml.AppendFormat(" StrokeDashArray=\"");
            xaml.AppendFormat("%g", shape.dashArray[0]);
            for (uint32_t i = 1; i < shape.dashArray.Size(); i++)
                xaml.AppendFormat(" %g", shape.dashArray[i]);
            xaml.AppendFormat("\"");
        }

        if (shape.dashOffet != 0.0f)
        {
            xaml.AppendFormat(" StrokeDashOffset=\"%g\"", shape.dashOffet);
        }

        if (shape.lineCap != PenLineCap_Flat)
        {
            const char* lineCap = shape.lineCap == PenLineCap_Square ? "Square" : "Round";
            xaml.AppendFormat(" StrokeStartLineCap=\"%s\" StrokeEndLineCap=\"%s\""
                " StrokeDashCap=\"%s\"", lineCap, lineCap, lineCap);
        }

        // LineJoin
        if (shape.lineJoin != PenLineJoin_Miter)
        {
            if (shape.lineJoin == PenLineJoin_Bevel)
            {
                xaml.AppendFormat(" StrokeLineJoin=\"Bevel\"");
            }
            else if (shape.lineJoin == PenLineJoin_Round)
            {
                xaml.AppendFormat(" StrokeLineJoin=\"Round\"");
            }
        }

        if (shape.opacity != 1.0f)
        {
            xaml.AppendFormat(" Opacity=\"%g\"", shape.opacity);
        }

        if (shape.mtx != Transform2::Identity())
        {
            xaml.AppendFormat(" RenderTransform=\"%g %g %g %g %g %g\"", shape.mtx[0][0],
                shape.mtx[0][1], shape.mtx[1][0], shape.mtx[1][1], shape.mtx[2][0],
                shape.mtx[2][1]);
        }

        if (IsGradientBrush(shape.fill) || IsGradientBrush(shape.stroke))
        {
            xaml.Append(">\n");
            WriteBrushes(xaml, shape);
            xaml.Append("  </Path>\n");
        }
        else
        {
            xaml += "/>\n";
        }
    }

    xaml += "</Canvas>";
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::LoadXAML(const Uri& uri)
{
    mErrorList->Clear();

    FixedString<512> filename;
    uri.GetPath(filename);

    if (StrCaseEndsWith(filename.Str(), ".xaml"))
    {
        SetContent(uri, GUI::LoadXaml(uri));
    }

    if (IsImage(filename.Str()))
    {
        Ptr<Image> root = MakePtr<Image>();
        root->SetSource(MakePtr<BitmapImage>(uri));
        SetContent(uri, root);
    }

    if (IsFont(filename.Str()))
    {
        Ptr<Stream> stream = OpenFileStream(filename.Str());

        Vector<Typeface> faces;
        Fonts::GetTypefaces(stream, [&faces](const Typeface& typeface)
        {
            faces.PushBack(typeface);
        });

        if (!faces.Empty())
        {
            int off = Max(StrFindLast(filename.Str(), "/"), StrFindLast(filename.Str(), "\\"));
            off = off == -1 ? 0 : off + 1;

            String family(filename.Str(), off);
            family += "#";
            family += faces[0].familyName;

            StrReplace((char*)family.Str(), '\\', '/');

            Ptr<StackPanel> root = MakePtr<StackPanel>();
            root->SetBackground(Brushes::White());

            {
                Ptr<TextBlock> info = MakePtr<TextBlock>();
                info->SetForeground(Brushes::Black());

                info->GetInlines()->Add(MakePtr<Run>("Name: "));
                info->GetInlines()->Add(MakePtr<Run>(faces[0].familyName));
                info->GetInlines()->Add(MakePtr<LineBreak>());

                info->GetInlines()->Add(MakePtr<Run>("Weight: "));
                info->GetInlines()->Add(MakePtr<Run>(Boxing::Box(faces[0].weight)->ToString().Str()));
                info->GetInlines()->Add(MakePtr<LineBreak>());

                info->GetInlines()->Add(MakePtr<Run>("Style: "));
                info->GetInlines()->Add(MakePtr<Run>(Boxing::Box(faces[0].style)->ToString().Str()));
                info->GetInlines()->Add(MakePtr<LineBreak>());

                info->GetInlines()->Add(MakePtr<Run>("Stretch: "));
                info->GetInlines()->Add(MakePtr<Run>(Boxing::Box(faces[0].stretch)->ToString().Str()));

                root->GetChildren()->Add(info);
            }

            root->GetChildren()->Add(MakePtr<Separator>());

            {
                Ptr<TextBlock> info = MakePtr<TextBlock>();
                info->SetForeground(Brushes::Black());
                info->SetFontSize(22.0f);
                info->SetFontFamily(MakePtr<FontFamily>(family.Str()));
                info->SetFontWeight(faces[0].weight);
                info->SetFontStyle(faces[0].style);
                info->SetFontStretch(faces[0].stretch);

                info->GetInlines()->Add(MakePtr<Run>("abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPRSTUVWXYZ"));
                info->GetInlines()->Add(MakePtr<LineBreak>());
                info->GetInlines()->Add(MakePtr<Run>("1234567890.:,;'\"(!?)+-*/="));

                root->GetChildren()->Add(info);
            }

            root->GetChildren()->Add(MakePtr<Separator>());

            Ptr<TextBlock> text = MakePtr<TextBlock>();
            text->SetMargin(Thickness(4));
            text->SetForeground(Brushes::Black());
            root->GetChildren()->Add(text);

            constexpr uint32_t Sizes[] = { 12, 18, 24, 36, 48, 60, 72 };

            for (uint32_t size : Sizes)
            {
                text->GetInlines()->Add(MakePtr<Run>(String(String::VarArgs(), "%d   ", size).Str()));

                Ptr<Run> run = MakePtr<Run>("The quick brown fox jumps over the lazy dog. 1234567890");
                run->SetFontSize((float)size);
                run->SetFontFamily(MakePtr<FontFamily>(family.Str()));
                run->SetFontWeight(faces[0].weight);
                run->SetFontStyle(faces[0].style);
                run->SetFontStretch(faces[0].stretch);

                text->GetInlines()->Add(run);
                text->GetInlines()->Add(MakePtr<LineBreak>());
            }

            SetContent(uri, root);
        }
    }

    if (StrCaseEndsWith(filename.Str(), ".riv"))
    {
        if (Factory::IsComponentRegistered(Symbol("NoesisGUIExtensions.RiveControl")))
        {
            constexpr char Template[] =
            R"(
                <Page xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                  xmlns:noesis="clr-namespace:NoesisGUIExtensions;assembly=Noesis.GUI.Extensions">
                  <noesis:RiveControl Source="%s"/>
                </Page>
            )";

            String xaml(String::VarArgs(), Template, uri.Str());
            SetContent(uri, GUI::ParseXaml<FrameworkElement>(xaml.Str()));
        }
    }

    if (StrCaseEndsWith(filename.Str(), ".svg"))
    {
        SVG::Image image;
        SVG::Parse(OpenFileStream(filename.Str()), image);

        String xaml;
        WriteXaml(xaml, image);
 
        SetContent(uri, GUI::ParseXaml<Canvas>(xaml.Str()));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::ClearErrors()
{
    mErrorList->Clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::SetContent(const Uri& uri, BaseComponent* content_)
{
    mContainer->SetChild(nullptr);

    FrameworkElement* content = DynamicCast<FrameworkElement*>(content_);

    if (content)
    {
        mContainer->SetChild(content);

        if (!StrEquals(uri.Str(), "Content.xaml"))
        {
            FixedString<512> filename;
            uri.GetPath(filename);

            UpdateTitle(filename.Str());
            mActiveUri = uri;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::OnPreviewMouseRightButtonDown(const MouseButtonEventArgs& e)
{
    ParentClass::OnPreviewMouseRightButtonDown(e);

    ModifierKeys m = GetKeyboard()->GetModifiers();
    Point position = mRoot->PointFromScreen(e.position);

    if (IsFileLoaded() && (m & (ModifierKeys_Control | ModifierKeys_Alt)) != 0)
    {
        CaptureMouse();

        if (m & ModifierKeys_Control)
        {
            mDragging = true;
        }
        else if (m & ModifierKeys_Alt)
        {
            mRotating = true;
        }

        mDraggingLastPosition = position;
        e.handled = true;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::OnPreviewMouseRightButtonUp(const MouseButtonEventArgs& e)
{
    ParentClass::OnPreviewMouseRightButtonUp(e);

    if (mDragging || mRotating)
    {
        ReleaseMouseCapture();

        mDragging = false;
        mRotating = false;

        e.handled = true;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::OnPreviewMouseMove(const MouseEventArgs& e)
{
    ParentClass::OnPreviewMouseMove(e);

    Point position = mRoot->PointFromScreen(e.position);

    if (mDragging)
    {
        float x = mContainerTranslate->GetX();
        float y = mContainerTranslate->GetY();

        Point delta = position - mDraggingLastPosition;
        mDraggingLastPosition = position;

        mContainerTranslate->SetX(x + delta.x);
        mContainerTranslate->SetY(y + delta.y);

        e.handled = true;
    }

    if (mRotating)
    {
        float x = mTransform3D->GetRotationY();
        float y = mTransform3D->GetRotationX();

        Point delta = position - mDraggingLastPosition;
        mDraggingLastPosition = position;

        mTransform3D->SetRotationY(x - delta.x * 0.5f);
        mTransform3D->SetRotationX(y + delta.y * 0.5f);

        e.handled = true;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::OnPreviewMouseWheel(const MouseWheelEventArgs& e)
{
    ParentClass::OnPreviewMouseWheel(e);

    if (IsFileLoaded() && (GetKeyboard()->GetModifiers() & ModifierKeys_Control) != 0)
    {
        mZoom = Clip(mZoom * (e.wheelRotation > 0 ? 1.05f : 0.952381f), 0.01f, 100.0f);

        float width = mContainer->GetActualWidth();
        float height = mContainer->GetActualHeight();

        Point center(width * 0.5f, height * 0.5f);
        Point position = mRoot->PointFromScreen(e.position);
        Point point = mContainer->PointFromScreen(e.position);
        Point pointScaled = center + (point - center) * mZoom;
        Point offset = position - pointScaled;

        mContainerScale->SetScaleX(mZoom);
        mContainerScale->SetScaleY(mZoom);

        if (IsZero(mTransform3D->GetRotationX()) && IsZero(mTransform3D->GetRotationY()))
        {
            mContainerTranslate->SetX(offset.x);
            mContainerTranslate->SetY(offset.y);
        }

        e.handled = true;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::OnPreviewKeyDown(const KeyEventArgs& e)
{
    if (e.key == Key_R && (GetKeyboard()->GetModifiers() & ModifierKeys_Control) != 0)
    {
        Reset();
        e.handled = true;
    }
    else if (e.key == Key_F5 && IsFileLoaded())
    {
        LoadXAML(mActiveUri);
        e.handled = true;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::OnFileDropped(const char* filename)
{
    LoadXAML(Uri::File(filename));
    Reset();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::ErrorHandler(const char* filename, uint32_t line, const char* desc, bool fatal)
{
    MainWindow* window = (MainWindow*)Application::Current()->GetMainWindow();
    window->mErrorList->Add(Boxing::Box(desc));
    window->mDefaultErrorHandler(filename, line, desc, fatal);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::InitializeComponent()
{
    GUI::LoadComponent(this, "XamlPlayer/MainWindow.xaml");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool MainWindow::ConnectField(BaseComponent* object, const char* name)
{
    NS_CONNECT_FIELD(mErrors, "Errors");
    NS_CONNECT_FIELD(mRoot, "LayoutRoot");
    NS_CONNECT_FIELD(mContainer, "Container");

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::UpdateTitle(const char* filename)
{
    char title[512] = "XamlPlayer ";

    if (!StrStartsWith(GetBuildVersion(), "0.0.0"))
    {
        StrAppend(title, sizeof(title), "v");
        StrAppend(title, sizeof(title), GetBuildVersion());
    }

    if (filename != nullptr)
    {
        StrAppend(title, sizeof(title), " - ");
        StrAppend(title, sizeof(title), filename);
    }

    SetTitle(title);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool MainWindow::IsFileLoaded() const
{
    return !StrIsEmpty(mActiveUri.Str());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::Reset()
{
    mZoom = 1.0f;

    mContainerScale->SetScaleX(1.0f);
    mContainerScale->SetScaleY(1.0f);
    mContainerTranslate->SetX(0.0f);
    mContainerTranslate->SetY(0.0f);

    mTransform3D->SetRotationX(0.0f);
    mTransform3D->SetRotationY(0.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::OnInitialized(BaseComponent*, const EventArgs&)
{
    UpdateTitle(nullptr);

    // Error list
    mErrorList = *new ObservableCollection<BaseComponent>();
    mErrors->SetItemsSource(mErrorList);
    mDefaultErrorHandler = SetErrorHandler(ErrorHandler);

    TransformGroup* group = (TransformGroup*)mContainer->GetRenderTransform();
    TransformCollection* children = group->GetChildren();
    NS_ASSERT(children->Count() == 2);

    mContainerScale = (ScaleTransform*)children->Get(0);
    NS_ASSERT(mContainerScale != 0);

    mContainerTranslate = (TranslateTransform*)children->Get(1);
    NS_ASSERT(mContainerTranslate != 0);

    mTransform3D = (CompositeTransform3D*)mContainer->GetTransform3D();
    NS_ASSERT(mTransform3D != 0);

    LoadXAML("Content.xaml");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::OnLoaded(BaseComponent*, const Noesis::RoutedEventArgs&)
{
    if (StrIsEmpty(mActiveUri.Str()))
    {
        PreviewToolbar();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION_(XamlPlayer::MainWindow, "XamlPlayer.MainWindow")

NS_END_COLD_REGION
