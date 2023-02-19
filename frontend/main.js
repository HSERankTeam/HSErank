var app

(async () => {
    journals_json = await (await window.journals_promise).json()

    app = new Vue({
        el: '#app',
        data: {
            journals: journals_json,
            universities: [{"name":"Высшая школа экономики", "city":"Samara", "metric1":12.6, "metric2": 41.7, "metric3": 3.0},
            {"name":"Высшая школа экономики", "city":"Samara", "metric1":12.6, "metric2": 41.7, "metric3": 3.0},
            {"name":"Высшая школа экономики", "city":"Samara", "metric1":12.6, "metric2": 41.7, "metric3": 3.0},
            {"name":"Высшая школа экономики", "city":"Samara", "metric1":12.6, "metric2": 41.7, "metric3": 3.0},
            {"name":"Высшая школа экономики", "city":"Samara", "metric1":12.6, "metric2": 41.7, "metric3": 3.0},
            {"name":"Высшая школа экономики", "city":"Samara", "metric1":12.6, "metric2": 41.7, "metric3": 3.0},
            {"name":"Высшая школа экономики", "city":"Samara", "metric1":12.6, "metric2": 41.7, "metric3": 3.0},
            {"name":"Высшая школа экономики", "city":"Samara", "metric1":12.6, "metric2": 41.7, "metric3": 3.0}]
        }
    })

})()
