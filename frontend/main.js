var app

(async () => {
    let years = [];
    let current_date = new Date();
    for (let i = 1970; i <= current_date.getFullYear(); i++) {
        years.push(i);
    }
    let reversed_years = [...years];
    reversed_years.reverse();

    let journals_json = await (await window.journals_promise).json()

    app = new Vue({
        el: '#app',
        data: {
            years: years,
            rev_years: reversed_years,
            journals: journals_json,
            universities: [{"name":"Высшая школа экономики", "city":"Samara", "metric1":12.6, "metric2": 41.7, "metric3": 3.0},
            {"name":"Высшая школа экономики", "city":"Samara", "metric1":12.6, "metric2": 41.7, "metric3": 3.0},
            {"name":"Высшая школа экономики", "city":"Samara", "metric1":12.6, "metric2": 41.7, "metric3": 3.0},
            {"name":"Высшая школа экономики", "city":"Samara", "metric1":12.6, "metric2": 41.7, "metric3": 3.0},
            {"name":"Высшая школа экономики", "city":"Samara", "metric1":12.6, "metric2": 41.7, "metric3": 3.0},
            {"name":"Высшая школа экономики", "city":"Samara", "metric1":12.6, "metric2": 41.7, "metric3": 3.0},
            {"name":"Высшая школа экономики", "city":"Samara", "metric1":12.6, "metric2": 41.7, "metric3": 3.0},
            {"name":"Высшая школа экономики", "city":"Samara", "metric1":12.6, "metric2": 41.7, "metric3": 3.0}]
        },
    })

})()
