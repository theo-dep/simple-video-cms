const zip = (...arrays) => {
    const minLength = Math.min(...arrays.map(arr => arr.length));
    return Array.from({ length: minLength }, (_, i) => arrays.map(arr => arr[i]));
};

document.addEventListener('DOMContentLoaded', () => {
    const openDrawers = document.getElementsByClassName('open-drawer');
    const closeDrawers = document.getElementsByClassName('drawer-close');
    const drawers = document.getElementsByName('drawer-checkbox');
    const drawerOverlays = document.getElementsByClassName('drawer-overlay');

    let id = 0;
    for (const [openDrawer, closeDrawer, drawer, drawerOverlay] of zip(openDrawers, closeDrawers, drawers, drawerOverlays)) {
        // increment ids
        openDrawer.setAttribute('for', `${openDrawer.getAttribute('for')}-${id}`);
        closeDrawer.setAttribute('for', `${closeDrawer.getAttribute('for')}-${id}`);
        drawerOverlay.setAttribute('for', `${drawerOverlay.getAttribute('for')}-${id}`);
        drawer.setAttribute('id', `${drawer.getAttribute('id')}-${id}`);
        id++;

        let wasPopped = false;

        drawer.addEventListener('click', () => {
            if (drawer.checked) {
                history.pushState(null, '');
            } else if (!wasPopped) {
                // click on close or click on overlay
                history.back();
            }
            wasPopped = false;
        });

        window.addEventListener('popstate', () => {
            if (drawer.checked) {
                wasPopped = true;
                drawer.click();
            }
        });
    }
});
