import QtQuick 2.15
import QtQuick.Controls 2.15

Menu {
    id: contentMenu
    focus: false
    property alias model: contentMenuList.model

    height: 32 * contentMenuList.count

    signal itemClicked(var itemId)

    background: Rectangle {
        border.width: 1
        border.color: settings.colorStyle? "#cccccc":"#101010"
        radius: 0
        color: settings.colorStyle? "#eeeeee":"#151515"
        implicitWidth: 200
        implicitHeight: 40
    }

    contentItem: ListView {
        id: contentMenuList
        height: 32*count
        delegate: PushButton {
            showIcon: true
            text: itemText
            width: ListView.view.width
            flat: true
            height: 32
            border.color: "transparent"
            property var itemStoreId: itemId
            onClicked: {
                contentMenu.itemClicked(itemStoreId);
                contentMenu.close();
            }
        }
    }
}
