document.addEventListener('DOMContentLoaded', () => {
    const openDrawerButton = document.getElementById('drawer-checkbox');
    if (openDrawerButton === null) {
        return;
    }

    let wasPopped = false;

    openDrawerButton.addEventListener('click', () => {
        if (openDrawerButton.checked) {
            history.pushState(null, '');
        } else if (!wasPopped) {
            // click on close or click on overlay
            history.back();
        }
        wasPopped = false;
    });

    window.addEventListener('popstate', () => {
        if (openDrawerButton.checked) {
            wasPopped = true;
            openDrawerButton.click();
        }
    });
});
